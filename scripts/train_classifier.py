#!/usr/bin/env python3
"""Train the local linear toxicity classifier for kagami moderation.

This script trains a thin linear classifier (one logistic regression per
moderation axis) on top of a pre-existing sentence embedding model, then
exports the resulting weights as a binary file that the kagami server
bundles via cmake/EmbedAssets.cmake and loads at runtime through
LocalClassifierLayer.

The classifier is intentionally tiny: for each embedding (default 384
dims for bge-small-en-v1.5) it performs one matrix-vector multiply and
one sigmoid per axis. Total inference cost is ~1-5µs per message on
modern ARM, vs 400-800ms for the OpenAI moderation API call it
replaces. It is NOT as accurate as the remote classifier, but it is
accurate enough to short-circuit the obvious cases (high confidence
clean, high confidence bad) and escalate the uncertain middle to the
remote layer.

==============================================================================
Which embedding model is this for?
==============================================================================

The classifier weights are ONLY meaningful in the latent space of the
specific embedding model they were trained against. Swapping the
runtime embedding model invalidates the weights — the dimensionality
might still happen to match (384 for both bge-small and all-MiniLM for
example) so you would not even get a loader error, you would just get
silently wrong classifications.

The weights file format stores the full HuggingFace model identifier in
the header. At load time, kagami compares that identifier against the
currently-configured content_moderation.embeddings.hf_model_id. On
mismatch, it emits a WARNING log line and disables the local classifier
layer — the moderation stack keeps working with the remaining layers,
the operator gets a clear message pointing at this training script.

If you are seeing that warning, the fix is: update the
--embedding-model arg below to match the runtime model, re-run this
script, rebuild the Docker image.

==============================================================================
Datasets
==============================================================================

The default configuration combines two public toxicity datasets:

  1. **Jigsaw Toxic Comment Classification Challenge** (from Kaggle).
     ~160k crowd-labeled Wikipedia comments with 6 binary axes: toxic,
     severe_toxic, obscene, threat, insult, identity_hate. Broad
     coverage of toxicity/hate/violence but no label for sexual content.

  2. **allenai/real-toxicity-prompts** (from HuggingFace, public).
     ~100k OpenWebText excerpts scored by Perspective API on 7 axes
     including **sexually_explicit** (the big gap in Jigsaw), plus
     toxicity/severe_toxicity/insult/identity_attack/threat/profanity/
     flirtation. Each row has a "prompt" and a "continuation" text
     each with their own attribute scores — we treat them as separate
     training examples for ~2x effective rows (~200k).

Combining them gives the classifier training signal on 4 out of the
8 kagami axes:
    toxicity        — Jigsaw(toxic OR severe_toxic) + RTP(tox>0.5 OR insult>0.5 OR profanity>0.5)
    hate            — Jigsaw(identity_hate)          + RTP(identity_attack>0.5)
    sexual          — (Jigsaw: no data)              + RTP(sexually_explicit>0.5)
    violence        — Jigsaw(threat)                 + RTP(threat>0.5)

The remaining 4 axes (sexual_minors, self_harm, visual_noise, link_risk)
have no training source in these datasets. Their weight rows stay
zero and their biases are initialized to -20 so sigmoid(-20) ≈ 2e-9
contributes effectively nothing to max_confidence at runtime — the
kagami moderation stack handles those axes through other layers
(slur filter for explicit content, UrlExtractor for link_risk,
UnicodeClassifier for visual_noise).

==============================================================================
Prerequisites
==============================================================================

- Python 3.10+ with pip
- Kaggle API credentials (for Jigsaw) — either ~/.kaggle/kaggle.json,
  KAGGLE_USERNAME/KAGGLE_KEY env vars, or KAGGLE_API_TOKEN env var.
  See https://www.kaggle.com/docs/api#authentication
  IMPORTANT: you must also accept the Jigsaw competition rules once in
  a browser at the competition rules page, otherwise the API returns
  HTTP 403 regardless of credentials.
- ~1 GB disk space for the downloaded datasets and embedding cache
- ~15-30 min runtime on CPU (the embedding step dominates), or
  ~10-15 min on Apple Silicon MPS with --batch-size 128.

Install dependencies:

    pip install \\
        numpy \\
        pandas \\
        scikit-learn \\
        sentence-transformers \\
        datasets \\
        kaggle

==============================================================================
Usage
==============================================================================

    # Default: combined Jigsaw + real-toxicity-prompts
    python3 scripts/train_classifier.py

    # Single dataset only
    python3 scripts/train_classifier.py --dataset jigsaw
    python3 scripts/train_classifier.py --dataset rtp

    # Override the embedding model (remember to change kagami config too)
    python3 scripts/train_classifier.py \\
        --embedding-model sentence-transformers/all-MiniLM-L6-v2

    # Limit training data for a quick sanity run
    python3 scripts/train_classifier.py --max-rows 10000

    # Custom output location
    python3 scripts/train_classifier.py \\
        --output /tmp/classifier-weights-dev.bin

    # Apple Silicon: use MPS with a bigger batch for speedup
    python3 scripts/train_classifier.py --device mps --batch-size 128

==============================================================================
Output format
==============================================================================

The output file is binary little-endian, layout:

    offset  size    field
    ------  ----    -----
    0       8       magic "KGCLF\\x01\\x00\\x00"
    8       4       format version (uint32): currently 1
    12      4       num_categories (uint32): currently 8
    16      4       embedding_dim (uint32): e.g. 384
    20      4       model_name_len (uint32)
    24      N       model_name (utf-8, not null-terminated)
    24+N    M       weights (float32, row-major, num_categories x dim)
    24+N+M  P       biases (float32, num_categories)

Kagami's LocalClassifierLayer::load_weights parses this header,
validates magic/version/dimension, compares model_name against the
runtime embedding model id, and only activates the layer on a full
match. See include/moderation/LocalClassifierLayer.h for the loader.

==============================================================================
Jigsaw → kagami axis mapping
==============================================================================

Jigsaw Toxic Comment Classification Challenge labels 6 binary axes:
toxic, severe_toxic, obscene, threat, insult, identity_hate. Kagami's
moderation stack uses 8 axes — some map directly, some are left at 0
because other layers handle them:

    kagami axis      <- jigsaw label(s)
    ---------------  -----------------------------------------
    toxicity         toxic OR severe_toxic
    hate             identity_hate
    sexual           (no direct jigsaw label — left at 0)
    sexual_minors    (no direct jigsaw label — left at 0)
    violence         threat
    self_harm        (no direct jigsaw label — left at 0)
    visual_noise     (handled by UnicodeClassifier, left at 0)
    link_risk        (handled by UrlExtractor, left at 0)

The "left at 0" axes still appear as weight rows in the output file
(zeroed out) so the runtime C++ code can use a fixed-stride layout.
Messages that would trip those axes will fall through to the other
layers in the stack rather than to this classifier.
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


# Kagami's 8 moderation axes in their canonical order. The C++ loader
# expects the weights in this exact row order. Do not reorder without
# also updating LocalClassifierLayer::load_weights.
KAGAMI_AXES = [
    "hate",
    "sexual",
    "sexual_minors",
    "violence",
    "self_harm",
    "visual_noise",  # zeroed
    "link_risk",     # zeroed
]

DEFAULT_EMBEDDING_MODEL = "ChristianAzinn/bge-small-en-v1.5-gguf"
DEFAULT_OUTPUT_V1 = "assets/moderation/classifier-weights-v1.bin"
DEFAULT_OUTPUT_V2 = "assets/moderation/classifier-weights-v2.bin"
SENTINEL_BIAS = -20.0
HIDDEN_DIM = 64

# The HF model id we pass to sentence-transformers might differ from
# the one kagami uses at runtime (kagami loads GGUF files via
# llama.cpp, sentence-transformers uses the base HF repo). They must
# reference the same underlying bge-small-en-v1.5 weights. The
# embedding runtime on kagami's side loads from:
#   ChristianAzinn/bge-small-en-v1.5-gguf
# which is a GGUF quantized repack of:
#   BAAI/bge-small-en-v1.5
# The underlying model is the same; the GGUF is just a different
# serialization of the same weights.
SENTENCE_TRANSFORMER_ID = "BAAI/bge-small-en-v1.5"

# Map GGUF repo names to their sentence-transformers equivalents.
# The GGUF is just a different serialization; the underlying model is the same.
GGUF_TO_ST = {
    "ChristianAzinn/bge-small-en-v1.5-gguf": "BAAI/bge-small-en-v1.5",
    "mykor/paraphrase-multilingual-MiniLM-L12-v2.gguf": "sentence-transformers/paraphrase-multilingual-MiniLM-L12-v2",
}


def die(msg: str, code: int = 1) -> "NoReturn":  # noqa: F821
    print(f"FATAL: {msg}", file=sys.stderr)
    sys.exit(code)


def require(*modules: str) -> None:
    missing = []
    for m in modules:
        try:
            __import__(m)
        except ImportError:
            missing.append(m)
    if missing:
        die(
            "Missing Python dependencies: "
            + ", ".join(missing)
            + "\nInstall with: pip install numpy pandas scikit-learn "
              "sentence-transformers kaggle"
        )


def download_jigsaw(target_dir: Path) -> Path:
    """Download and extract the Jigsaw training CSV.

    Returns the path to train.csv. Idempotent — skips download if
    already present.
    """
    train_csv = target_dir / "train.csv"
    if train_csv.exists():
        print(f"[1/5] Jigsaw train.csv already present at {train_csv}")
        return train_csv

    target_dir.mkdir(parents=True, exist_ok=True)

    print("[1/5] Downloading Jigsaw Toxic Comment Classification dataset "
          "from Kaggle...")
    # Prefer the kaggle CLI over the Python API because the CLI exits
    # with a clear nonzero status on credential failure.
    try:
        subprocess.run(
            [
                "kaggle", "competitions", "download",
                "-c", "jigsaw-toxic-comment-classification-challenge",
                "-p", str(target_dir),
            ],
            check=True,
        )
    except FileNotFoundError:
        die(
            "`kaggle` CLI not found. Install with `pip install kaggle` "
            "and set up credentials at ~/.kaggle/kaggle.json. See "
            "https://www.kaggle.com/docs/api#authentication"
        )
    except subprocess.CalledProcessError as e:
        die(
            f"Kaggle download failed (exit {e.returncode}). Most "
            "likely cause: missing or unaccepted competition rules. "
            "Visit the competition page in a browser and click "
            "'I Understand and Accept' before retrying:\n"
            "  https://www.kaggle.com/competitions/"
            "jigsaw-toxic-comment-classification-challenge/rules"
        )

    # Extract the outer zip and the nested train.csv.zip.
    zip_path = target_dir / "jigsaw-toxic-comment-classification-challenge.zip"
    if not zip_path.exists():
        die(f"expected {zip_path} after download, not found")
    subprocess.run(["unzip", "-o", str(zip_path), "-d", str(target_dir)], check=True)
    nested_zip = target_dir / "train.csv.zip"
    if nested_zip.exists():
        subprocess.run(["unzip", "-o", str(nested_zip), "-d", str(target_dir)], check=True)
    if not train_csv.exists():
        die(f"expected {train_csv} after extract, not found")
    return train_csv


def map_jigsaw_to_kagami_labels(df):
    """Map Jigsaw's 6 binary labels to kagami's 8 axes.

    Returns an (N, 7) numpy float32 array. Axes with no direct Jigsaw
    source stay at 0.
    """
    import numpy as np

    labels = np.zeros((len(df), len(KAGAMI_AXES)), dtype=np.float32)
    axis_idx = {name: i for i, name in enumerate(KAGAMI_AXES)}

    # hate: identity_hate in Jigsaw's schema
    labels[:, axis_idx["hate"]] = df["identity_hate"].values.astype(np.float32)
    # violence: threat
    labels[:, axis_idx["violence"]] = df["threat"].values.astype(np.float32)
    # sexual / sexual_minors / self_harm / visual_noise / link_risk: 0

    return labels


# Threshold for converting RTP continuous Perspective-API scores
# (0.0 to 1.0) to binary labels for logistic regression training.
# 0.5 is the standard calibration point for Perspective.
RTP_THRESHOLD = 0.5


def load_jigsaw_arrays(args, require=None):
    """Load Jigsaw as (texts: list[str], labels: np.ndarray Nx8)."""
    import pandas as pd

    dataset_dir = Path(args.dataset_dir).expanduser().resolve()
    train_csv = download_jigsaw(dataset_dir)
    print(f"      loading {train_csv}")
    df = pd.read_csv(train_csv)
    labels = map_jigsaw_to_kagami_labels(df)
    texts = df["comment_text"].astype(str).tolist()
    print(f"      jigsaw: {len(texts):,} rows")
    return texts, labels


def load_rtp_arrays(args, require=None):
    """Load allenai/real-toxicity-prompts as (texts, labels Nx8).

    Each dataset row yields TWO training examples: one for the
    'prompt' field and one for the 'continuation' field, each with
    their own Perspective API scores. This roughly doubles the
    effective training-set size for free.
    """
    import numpy as np
    from datasets import load_dataset

    print("      loading allenai/real-toxicity-prompts from HF")
    ds = load_dataset("allenai/real-toxicity-prompts", split="train")

    axis_idx = {name: i for i, name in enumerate(KAGAMI_AXES)}
    texts = []
    labels_list = []

    def make_label_row(score_dict):
        row = np.zeros(len(KAGAMI_AXES), dtype=np.float32)
        # hate: identity_attack
        ia = score_dict.get("identity_attack") or 0.0
        row[axis_idx["hate"]] = 1.0 if ia > RTP_THRESHOLD else 0.0
        # sexual: sexually_explicit — RTP's unique contribution
        se = score_dict.get("sexually_explicit") or 0.0
        row[axis_idx["sexual"]] = 1.0 if se > RTP_THRESHOLD else 0.0
        # violence: threat
        thr = score_dict.get("threat") or 0.0
        row[axis_idx["violence"]] = 1.0 if thr > RTP_THRESHOLD else 0.0
        return row

    for row in ds:
        for field in ("prompt", "continuation"):
            obj = row.get(field)
            if not obj:
                continue
            text = obj.get("text")
            if not text or not isinstance(text, str):
                continue
            # Perspective scores can be None when the API declined
            # to score a fragment — skip those to avoid biasing the
            # all-zero labels.
            if obj.get("toxicity") is None:
                continue
            texts.append(text)
            labels_list.append(make_label_row(obj))

    labels = np.stack(labels_list, axis=0) if labels_list else np.zeros((0, len(KAGAMI_AXES)), dtype=np.float32)
    print(f"      rtp: {len(texts):,} rows (prompt+continuation)")
    return texts, labels


def load_toxigen_arrays(args, require=None):
    """Load toxigen/toxigen-data as (texts, labels Nx8).

    Toxigen is ~250k AI-generated hate speech snippets targeting
    specific demographic groups. Each row has:
      - prompt: context prompt used to generate the example
      - generation: the model-generated output (what we train on)
      - roberta_prediction: a RoBERTa classifier's toxicity score for
        the generation, in [0, 1]
      - group: the targeted demographic (e.g. "black", "asian")

    We use `generation` as the text and `roberta_prediction > 0.5`
    as the binary label for the `hate` axis.

    Toxigen's one-sided focus (hate only) complements Jigsaw+RTP
    by increasing positive-class density on the hate axis, which
    is the hardest axis to train on standard-web datasets because
    overt hate speech is relatively rare there.
    """
    import numpy as np
    from datasets import load_dataset

    print("      loading toxigen/toxigen-data from HF")
    ds = load_dataset("toxigen/toxigen-data", "train", split="train")

    axis_idx = {name: i for i, name in enumerate(KAGAMI_AXES)}
    texts = []
    labels_list = []

    for row in ds:
        gen = row.get("generation")
        if not gen or not isinstance(gen, str):
            continue
        score = row.get("roberta_prediction")
        if score is None:
            continue
        label = np.zeros(len(KAGAMI_AXES), dtype=np.float32)
        is_toxic = 1.0 if score > RTP_THRESHOLD else 0.0
        label[axis_idx["hate"]] = is_toxic
        texts.append(gen)
        labels_list.append(label)

    labels = np.stack(labels_list, axis=0) if labels_list else np.zeros((0, len(KAGAMI_AXES)), dtype=np.float32)
    print(f"      toxigen: {len(texts):,} rows")
    return texts, labels


def load_combined_arrays(args):
    """Load Jigsaw ∪ RTP ∪ (optionally) Toxigen as (texts, labels Nx8)."""
    import numpy as np

    j_texts, j_labels = load_jigsaw_arrays(args)
    r_texts, r_labels = load_rtp_arrays(args)
    texts = j_texts + r_texts
    labels = np.concatenate([j_labels, r_labels], axis=0)
    if args.add_toxigen:
        t_texts, t_labels = load_toxigen_arrays(args)
        texts = texts + t_texts
        labels = np.concatenate([labels, t_labels], axis=0)
    print(f"      combined: {len(texts):,} rows")
    return texts, labels


def train(args) -> int:
    require("numpy", "sklearn", "sentence_transformers")
    if not args.ao_labels:
        require("pandas")
        if args.dataset in ("rtp", "toxigen", "combined"):
            require("datasets")

    import numpy as np
    from sentence_transformers import SentenceTransformer
    from sklearn.linear_model import LogisticRegression
    from sklearn.metrics import roc_auc_score
    from sklearn.model_selection import train_test_split

    # source_mask tracks data provenance: 0=real, 1=synthetic
    # Used for stratified evaluation so we can report AUC on real data
    # separately from synthetic (synthetic AUC is inflated due to
    # stylistic separability between LLM-generated and real chat text).
    source_mask = None

    if args.ao_labels:
        print(f"[2/5] Loading AO labels from {args.ao_labels}")
        texts, labels = load_ao_labels(args.ao_labels, None)
        source_mask = np.zeros(len(texts), dtype=np.int8)
        if args.synthetic:
            print(f"      Merging synthetic data from {args.synthetic}")
            syn_texts, syn_labels = load_ao_labels(args.synthetic, None)
            if len(syn_texts) > 0:
                texts = texts + syn_texts
                labels = np.concatenate([labels, syn_labels], axis=0)
                source_mask = np.concatenate([
                    source_mask, np.ones(len(syn_texts), dtype=np.int8)
                ])
                print(f"      merged total: {len(texts)} rows "
                      f"({len(texts) - len(syn_texts)} real + {len(syn_texts)} synthetic)")
    else:
        label = f"{args.dataset}"
        if args.dataset == "combined" and args.add_toxigen:
            label += "+toxigen"
        print(f"[2/5] Loading dataset(s): {label}")
        if args.dataset == "jigsaw":
            texts, labels = load_jigsaw_arrays(args)
        elif args.dataset == "rtp":
            texts, labels = load_rtp_arrays(args)
        elif args.dataset == "toxigen":
            texts, labels = load_toxigen_arrays(args)
        elif args.dataset == "combined":
            texts, labels = load_combined_arrays(args)
        else:
            die(f"unknown --dataset value: {args.dataset}")

    # Apply --max-rows AFTER concatenation so the subsample is a
    # representative mix of whichever datasets were combined. Shuffle
    # first using a deterministic seed so the subsample is consistent
    # across re-runs and the split in train_test_split below is stable.
    if args.max_rows and len(texts) > args.max_rows:
        rng = np.random.default_rng(42)
        idx = rng.permutation(len(texts))[: args.max_rows]
        texts = [texts[i] for i in idx]
        labels = labels[idx]
    print(f"      {len(texts):,} rows after filtering")

    # Device selection: prefer Apple Metal (MPS) on Apple Silicon or
    # CUDA on Linux. Falls back to CPU. MPS speeds up a 160k-row
    # bge-small embedding pass from ~75 minutes on M-series CPU to
    # ~8 minutes on the same chip's GPU cores — worth the one-line
    # detection.
    import torch
    if args.device:
        device = args.device
    elif hasattr(torch.backends, "mps") and torch.backends.mps.is_available():
        device = "mps"
    elif torch.cuda.is_available():
        device = "cuda"
    else:
        device = "cpu"
    # Resolve the sentence-transformers model name from the embedding-model flag
    if args.st_model:
        st_model_id = args.st_model
    else:
        st_model_id = GGUF_TO_ST.get(args.embedding_model, SENTENCE_TRANSFORMER_ID)
    print(f"[3/5] Loading embedding model {st_model_id!r} on {device}")
    print(f"      (weight header will store {args.embedding_model!r})")
    model = SentenceTransformer(st_model_id, device=device)
    # get_embedding_dimension is the supported name in
    # sentence-transformers 5.x; fall back to the older name for
    # 3.x / 4.x users.
    if hasattr(model, "get_embedding_dimension"):
        embedding_dim = model.get_embedding_dimension()
    else:
        embedding_dim = model.get_sentence_embedding_dimension()
    print(f"      embedding dim = {embedding_dim}")

    # Batch size: 256 is aggressive but fits in memory for
    # bge-small (384-dim encoder, ~30 MB model) even on CPU. Larger
    # batches amortize per-batch Python overhead and (on MPS) GPU
    # kernel launch cost, which dominates at batch=64 for such a
    # small model.
    batch_size = args.batch_size
    print(f"[4/5] Embedding {len(texts):,} comments "
          f"(batch={batch_size}, device={device})...")
    X = model.encode(
        texts,
        batch_size=batch_size,
        convert_to_numpy=True,
        normalize_embeddings=True,
        show_progress_bar=True,
    ).astype(np.float32)

    # Train/val split for AUC reporting. Carry source_mask through
    # so we can report AUC stratified by real vs synthetic.
    if source_mask is not None:
        X_train, X_val, y_train, y_val, src_train, src_val = train_test_split(
            X, labels, source_mask, test_size=0.1, random_state=42
        )
    else:
        X_train, X_val, y_train, y_val = train_test_split(
            X, labels, test_size=0.1, random_state=42
        )
        src_val = None

    if args.architecture == "mlp":
        return train_mlp(args, X_train, X_val, y_train, y_val, embedding_dim, src_val)
    return train_linear(args, X_train, X_val, y_train, y_val, embedding_dim)


def train_linear(args, X_train, X_val, y_train, y_val, embedding_dim) -> int:
    """v1 logistic regression training (legacy path)."""
    import numpy as np
    from sklearn.linear_model import LogisticRegression
    from sklearn.metrics import roc_auc_score

    print("[5/5] Training one logistic regression per axis (v1 linear)")
    weights = np.zeros((len(KAGAMI_AXES), embedding_dim), dtype=np.float32)
    biases = np.full(len(KAGAMI_AXES), SENTINEL_BIAS, dtype=np.float32)

    for i, axis in enumerate(KAGAMI_AXES):
        y_train_i = y_train[:, i]
        y_val_i = y_val[:, i]
        valid_train = ~np.isnan(y_train_i)
        valid_val = ~np.isnan(y_val_i)
        y_tr = y_train_i[valid_train]
        y_va = y_val_i[valid_val]
        pos_count = int(y_tr.sum())
        total = len(y_tr)

        if pos_count < 5 or total < 20:
            print(f"      {axis:16s}: SKIPPED (pos={pos_count}, total={total})")
            continue

        clf = LogisticRegression(C=1.0, max_iter=200, random_state=42)
        clf.fit(X_train[valid_train], y_tr)

        try:
            auc = roc_auc_score(y_va, clf.predict_proba(X_val[valid_val])[:, 1])
        except ValueError:
            auc = float("nan")

        weights[i] = clf.coef_[0].astype(np.float32)
        biases[i] = float(clf.intercept_[0])
        print(f"      {axis:16s}: AUC={auc:.4f} (pos={pos_count}/{total})")

    output_path = Path(args.output).expanduser().resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    write_weights_file(
        output_path, len(KAGAMI_AXES), embedding_dim,
        args.embedding_model, weights, biases,
    )
    print(f"\nWrote {output_path} ({output_path.stat().st_size:,} bytes)")
    return 0


def train_mlp(args, X_train, X_val, y_train, y_val, embedding_dim,
              src_val=None) -> int:
    """v2 MLP training: per-axis 2-layer MLP with residual + Platt."""
    import numpy as np
    import torch
    import torch.nn as nn
    import torch.nn.functional as F
    import torch.optim as optim
    from sklearn.linear_model import LogisticRegression
    from sklearn.metrics import roc_auc_score, average_precision_score

    if torch.backends.mps.is_available():
        device = torch.device("mps")
    elif torch.cuda.is_available():
        device = torch.device("cuda")
    else:
        device = torch.device("cpu")

    print(f"[5/5] Training per-axis MLP (v2, hidden={HIDDEN_DIM}, device={device})")

    num_cat = len(KAGAMI_AXES)
    H = HIDDEN_DIM
    D = embedding_dim

    all_w1 = np.zeros((num_cat, D, H), dtype=np.float32)
    all_b1 = np.zeros((num_cat, H), dtype=np.float32)
    all_w2 = np.zeros((num_cat, H), dtype=np.float32)
    all_b2 = np.full(num_cat, SENTINEL_BIAS, dtype=np.float32)
    all_wskip = np.zeros((num_cat, D), dtype=np.float32)
    all_platt_a = np.ones(num_cat, dtype=np.float32)
    all_platt_b = np.zeros(num_cat, dtype=np.float32)

    class AxisMLP(nn.Module):
        def __init__(self):
            super().__init__()
            self.fc1 = nn.Linear(D, H)
            self.fc2 = nn.Linear(H, 1)
            self.skip = nn.Linear(D, 1, bias=False)
        def forward(self, x):
            h = F.relu(self.fc1(x))
            return (self.fc2(h) + self.skip(x)).squeeze(-1)

    for i, axis in enumerate(KAGAMI_AXES):
        y_train_i = y_train[:, i]
        y_val_i = y_val[:, i]
        valid_train = ~np.isnan(y_train_i)
        valid_val = ~np.isnan(y_val_i)
        y_tr = y_train_i[valid_train].astype(np.float32)
        y_va = y_val_i[valid_val].astype(np.float32)
        X_tr = X_train[valid_train]
        X_va = X_val[valid_val]
        n_pos = int(y_tr.sum())

        if n_pos < 5 or len(y_tr) < 20:
            print(f"      {axis:16s}: SENTINEL (pos={n_pos}, total={len(y_tr)})")
            continue

        torch.manual_seed(42)
        model = AxisMLP().to(device)
        X_t = torch.from_numpy(X_tr).float().to(device)
        y_t = torch.from_numpy(y_tr).float().to(device)
        X_v = torch.from_numpy(X_va).float().to(device)
        y_v = torch.from_numpy(y_va).float().to(device)

        n_neg = max(float(len(y_tr) - n_pos), 1.0)
        pos_weight = torch.tensor([n_neg / max(n_pos, 1)], device=device)
        criterion = nn.BCEWithLogitsLoss(pos_weight=pos_weight)
        # AdamW: decoupled weight decay (not L2 penalty baked into gradient).
        # Critical for grokking — L2 in Adam doesn't actually push weights
        # toward zero the same way because the adaptive learning rate
        # rescales the penalty per-parameter.
        optimizer = optim.AdamW(model.parameters(), lr=args.lr,
                                weight_decay=args.weight_decay)

        max_epochs = args.epochs
        max_patience = args.patience
        # Early stop on val AUC (not val loss). In the grokking regime,
        # val loss can increase (weight decay penalty grows) while val
        # AUC improves (decision boundary sharpens). AUC is what we
        # actually care about — ranking quality, not loss magnitude.
        best_val_auc = -1.0
        best_state = None
        stale_epochs = 0
        has_both_classes_val = len(np.unique(y_va)) >= 2

        for epoch in range(max_epochs):
            model.train()
            optimizer.zero_grad()
            train_loss = criterion(model(X_t), y_t)
            train_loss.backward()
            optimizer.step()

            # Evaluate val AUC periodically (every epoch is cheap on this scale)
            model.eval()
            with torch.no_grad():
                val_logits = model(X_v).cpu().numpy()
            val_probs = 1.0 / (1.0 + np.exp(-val_logits))

            if has_both_classes_val:
                val_auc = roc_auc_score(y_va, val_probs)
                if val_auc > best_val_auc + 1e-4:
                    best_val_auc = val_auc
                    best_state = {k: v.detach().clone() for k, v in model.state_dict().items()}
                    stale_epochs = 0
                else:
                    stale_epochs += 1
            else:
                # Can't compute AUC — fall back to val loss for early stopping
                val_loss = criterion(model(X_v), y_v).item()
                if best_val_auc < 0:  # never set
                    best_val_auc = -val_loss  # hack: use negative loss as proxy
                    best_state = {k: v.detach().clone() for k, v in model.state_dict().items()}
                stale_epochs += 1

            # Progress logging (every 100 epochs, or grok milestones)
            if epoch % 100 == 0 or (epoch > 0 and epoch % 500 == 0):
                w_norm = sum(p.data.norm().item() ** 2 for p in model.parameters()) ** 0.5
                print(f"        epoch {epoch:>5d}: train_loss={train_loss.item():.4f} "
                      f"val_auc={best_val_auc:.4f} w_norm={w_norm:.2f} "
                      f"stale={stale_epochs}")

            if stale_epochs >= max_patience:
                break

        if best_state:
            model.load_state_dict(best_state)
        model.eval()
        final_epoch = epoch + 1
        print(f"        stopped at epoch {final_epoch} "
              f"(best val_auc={best_val_auc:.4f})")

        # Platt calibration
        with torch.no_grad():
            val_logits = model(X_v).cpu().numpy()
        if len(np.unique(y_va)) >= 2:
            platt = LogisticRegression(C=1.0)
            platt.fit(val_logits.reshape(-1, 1), y_va)
            pa, pb = float(platt.coef_[0][0]), float(platt.intercept_[0])
            cal_probs = 1.0 / (1.0 + np.exp(-(pa * val_logits + pb)))
        else:
            pa, pb = 1.0, 0.0
            cal_probs = 1.0 / (1.0 + np.exp(-val_logits))

        try:
            auc = roc_auc_score(y_va, cal_probs)
        except ValueError:
            auc = float("nan")
        try:
            pr_auc = average_precision_score(y_va, cal_probs)
        except ValueError:
            pr_auc = float("nan")
        brier = float(np.mean((y_va - cal_probs) ** 2))

        sd = {k: v.cpu().numpy() for k, v in model.state_dict().items()}
        all_w1[i] = sd["fc1.weight"].T  # [D, H] from [H, D]
        all_b1[i] = sd["fc1.bias"]
        all_w2[i] = sd["fc2.weight"].squeeze(0)
        all_b2[i] = sd["fc2.bias"].item()
        all_wskip[i] = sd["skip.weight"].squeeze(0)
        all_platt_a[i] = pa
        all_platt_b[i] = pb

        print(f"      {axis:16s}: AUC={auc:.4f}  PR-AUC={pr_auc:.4f}  "
              f"Brier={brier:.4f}  (pos={n_pos}/{len(y_tr)})")

        # Stratified eval: report AUC on real vs synthetic separately
        if src_val is not None:
            src_va = src_val[valid_val]
            real_mask = src_va == 0
            syn_mask = src_va == 1
            for label, mask in [("real", real_mask), ("synth", syn_mask)]:
                if mask.sum() < 2 or len(np.unique(y_va[mask])) < 2:
                    continue
                try:
                    s_auc = roc_auc_score(y_va[mask], cal_probs[mask])
                    s_pr = average_precision_score(y_va[mask], cal_probs[mask])
                except ValueError:
                    continue
                n_pos_s = int(y_va[mask].sum())
                print(f"        {label:>5s}: AUC={s_auc:.4f}  "
                      f"PR-AUC={s_pr:.4f}  (pos={n_pos_s}/{int(mask.sum())})")

    output_path = Path(args.output).expanduser().resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    write_weights_file_v2(
        output_path, num_cat, D, H, args.embedding_model,
        all_w1, all_b1, all_w2, all_b2, all_wskip,
        all_platt_a, all_platt_b,
    )
    print(f"\nWrote {output_path} ({output_path.stat().st_size:,} bytes)")
    return 0


def load_ao_labels(db_path: str, embedding_model):
    """Load AO-specific labels from the labeler SQLite database.

    Returns (texts, labels) where labels is (N, 8) float32 with values
    0.0 (clean) or 1.0 (bad). Borderline labels (1) are excluded.
    Axes without labels get NaN so the per-axis training can skip them.
    """
    import numpy as np
    import sqlite3

    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row

    # Get all unique text_hashes that have at least one non-borderline label
    hashes_with_labels = conn.execute(
        "SELECT DISTINCT text_hash FROM labels WHERE label IN (0, 2)"
    ).fetchall()
    hashes = [r["text_hash"] for r in hashes_with_labels]

    if not hashes:
        print(f"      no labels found in {db_path}")
        return [], np.zeros((0, len(KAGAMI_AXES)), dtype=np.float32)

    # Build per-hash label vectors
    texts = []
    label_rows = []
    for h in hashes:
        msg = conn.execute("SELECT text FROM messages WHERE text_hash = ? LIMIT 1", (h,)).fetchone()
        if msg is None:
            continue
        texts.append(msg["text"])
        row = np.full(len(KAGAMI_AXES), np.nan, dtype=np.float32)
        for lr in conn.execute("SELECT axis, label FROM labels WHERE text_hash = ? AND label IN (0, 2)", (h,)):
            axis_name = lr["axis"]
            if axis_name in KAGAMI_AXES:
                idx = KAGAMI_AXES.index(axis_name)
                row[idx] = 1.0 if lr["label"] == 2 else 0.0
        label_rows.append(row)

    conn.close()
    labels = np.stack(label_rows) if label_rows else np.zeros((0, len(KAGAMI_AXES)), dtype=np.float32)
    print(f"      loaded {len(texts)} labeled messages from {db_path}")
    for i, axis in enumerate(KAGAMI_AXES):
        valid = ~np.isnan(labels[:, i])
        pos = int(labels[valid, i].sum()) if valid.any() else 0
        total = int(valid.sum())
        if total > 0:
            print(f"        {axis:16s}: {pos} positive / {total} total")
    return texts, labels


def write_weights_file_v2(
    path,
    num_categories: int,
    embedding_dim: int,
    hidden_dim: int,
    model_name: str,
    w1, b1, w2, b2, w_skip,  # np.ndarrays
    platt_a, platt_b,         # np.ndarrays (num_categories,)
) -> None:
    """Serialize to kagami v2 MLP binary format.

    Format:
      magic "KGCLF\\x02\\x00\\x00" | version=2 | num_cat | dim | hidden_dim |
      name_len | name | W1 | b1 | W2 | b2 | W_skip |
      calibration_type(1) | platt_a | platt_b
    """
    import numpy as np

    model_name_bytes = model_name.encode("utf-8")
    tmp_fd, tmp_name = tempfile.mkstemp(
        dir=str(path.parent), prefix=".classifier-v2-", suffix=".tmp"
    )
    try:
        with os.fdopen(tmp_fd, "wb") as f:
            f.write(b"KGCLF\x02\x00\x00")
            f.write(struct.pack("<I", 2))  # format version
            f.write(struct.pack("<I", num_categories))
            f.write(struct.pack("<I", embedding_dim))
            f.write(struct.pack("<I", hidden_dim))
            f.write(struct.pack("<I", len(model_name_bytes)))
            f.write(model_name_bytes)
            f.write(w1.astype(np.float32).tobytes())
            f.write(b1.astype(np.float32).tobytes())
            f.write(w2.astype(np.float32).tobytes())
            f.write(b2.astype(np.float32).tobytes())
            f.write(w_skip.astype(np.float32).tobytes())
            f.write(struct.pack("B", 1))  # calibration_type = platt
            f.write(platt_a.astype(np.float32).tobytes())
            f.write(platt_b.astype(np.float32).tobytes())
        shutil.move(tmp_name, str(path))
    except Exception:
        try:
            os.unlink(tmp_name)
        except OSError:
            pass
        raise


def write_weights_file(
    path: Path,
    num_categories: int,
    embedding_dim: int,
    model_name: str,
    weights,  # np.ndarray (num_categories, embedding_dim) float32
    biases,   # np.ndarray (num_categories,) float32
) -> None:
    """Serialize to kagami's LocalClassifier binary format.

    See the docstring at the top of this file for the full layout.
    """
    import numpy as np

    assert weights.shape == (num_categories, embedding_dim), (
        f"weights shape {weights.shape} != "
        f"({num_categories}, {embedding_dim})"
    )
    assert biases.shape == (num_categories,), (
        f"biases shape {biases.shape} != ({num_categories},)"
    )
    assert weights.dtype == np.float32
    assert biases.dtype == np.float32

    model_name_bytes = model_name.encode("utf-8")
    if len(model_name_bytes) > 512:
        raise ValueError(
            "model_name too long (>512 bytes) — keep it to the "
            "standard HuggingFace namespace/name form"
        )

    # Atomic write via temp file in the same directory, then rename.
    tmp_fd, tmp_name = tempfile.mkstemp(
        dir=str(path.parent), prefix=".classifier-", suffix=".tmp"
    )
    try:
        with os.fdopen(tmp_fd, "wb") as f:
            f.write(b"KGCLF\x01\x00\x00")
            f.write(struct.pack("<I", 1))  # format version
            f.write(struct.pack("<I", num_categories))
            f.write(struct.pack("<I", embedding_dim))
            f.write(struct.pack("<I", len(model_name_bytes)))
            f.write(model_name_bytes)
            f.write(weights.tobytes(order="C"))  # row-major float32
            f.write(biases.tobytes(order="C"))
        shutil.move(tmp_name, str(path))
    except Exception:
        try:
            os.unlink(tmp_name)
        except OSError:
            pass
        raise


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--dataset", default="combined",
        choices=["jigsaw", "rtp", "toxigen", "combined"],
        help=(
            "Which dataset(s) to train on. 'jigsaw' = Kaggle "
            "Jigsaw Toxic Comments (160k, needs rules accepted). "
            "'rtp' = allenai/real-toxicity-prompts (~100k, public, "
            "adds sexual axis coverage). 'toxigen' = "
            "toxigen/toxigen-data (~250k hate-speech samples, "
            "public). 'combined' = jigsaw + rtp (default, ~260k "
            "rows). Add --add-toxigen to also include toxigen."
        ),
    )
    parser.add_argument(
        "--add-toxigen", action="store_true",
        help=(
            "When --dataset=combined, also include toxigen to "
            "boost hate-axis positive density. Adds ~250k rows and "
            "proportionally longer training time."
        ),
    )
    parser.add_argument(
        "--output", default=None,
        help="Output path for the weights file (default: auto from architecture)",
    )
    parser.add_argument(
        "--embedding-model", default=DEFAULT_EMBEDDING_MODEL,
        help=(
            "HuggingFace model identifier stored in the weights header. "
            "Must match the runtime kagami embeddings.hf_model_id."
        ),
    )
    parser.add_argument(
        "--st-model", default=None,
        help=(
            "Sentence-transformers model name used for computing embeddings "
            "during training. If not set, derived from --embedding-model by "
            "mapping known GGUF repos to their ST equivalents."
        ),
    )
    parser.add_argument(
        "--dataset-dir", default="./.kagami-train-data",
        help="Where to cache the downloaded Jigsaw dataset",
    )
    parser.add_argument(
        "--max-rows", type=int, default=0,
        help=(
            "If >0, randomly subsample this many rows from the training "
            "set. Useful for quick smoke tests (5000 rows → ~2 minutes)."
        ),
    )
    parser.add_argument(
        "--device", default=None,
        help=(
            "Override the torch device (e.g. 'cpu', 'mps', 'cuda'). "
            "Default auto-detects MPS on Apple Silicon, CUDA on Linux, "
            "else CPU."
        ),
    )
    parser.add_argument(
        "--batch-size", type=int, default=256,
        help=(
            "Batch size for the embedding pass. 256 is a good default "
            "for bge-small (small model, large batches amortize Python "
            "overhead). Lower to 64 if you see memory pressure."
        ),
    )
    parser.add_argument(
        "--architecture", default="mlp",
        choices=["linear", "mlp"],
        help=(
            "Classifier architecture. 'linear' = v1 logistic regression "
            "(single GEMV per axis, ~12 KB weights). 'mlp' = v2 2-layer "
            "MLP with residual skip + Platt calibration (~800 KB weights). "
            "Default: mlp."
        ),
    )
    parser.add_argument(
        "--epochs", type=int, default=500,
        help="Max training epochs per axis (default: 500). For grokking regime, use 3000-5000.",
    )
    parser.add_argument(
        "--patience", type=int, default=50,
        help="Early stopping patience — epochs without val AUC improvement before stopping (default: 50). Set very high (500+) for grokking.",
    )
    parser.add_argument(
        "--lr", type=float, default=1e-3,
        help="Learning rate (default: 1e-3).",
    )
    parser.add_argument(
        "--weight-decay", type=float, default=None,
        help="AdamW weight decay (default: 0.01 normal, 0.5 with --grok).",
    )
    parser.add_argument(
        "--grok", action="store_true",
        help="Grokking regime: high weight decay (0.5), long training (5000 epochs), "
             "patient early stopping (500), AdamW. Trains past memorization to find "
             "simpler generalizing solutions via weight compression.",
    )
    parser.add_argument(
        "--ao-labels", default=None,
        help=(
            "Path to the labeler SQLite database with AO-specific labels. "
            "When provided, trains on AO labels only (ignores --dataset). "
            "Typically ~/Documents/notes-AOSDL/labeler/data/labels.sqlite."
        ),
    )
    parser.add_argument(
        "--synthetic", default=None,
        help=(
            "Path to a second SQLite database with synthetic labeled data. "
            "Merged with --ao-labels data before training. Same schema. "
            "Typically ~/Documents/notes-AOSDL/labeler/data/synthetic.sqlite."
        ),
    )
    parser.add_argument(
        "--eval-synthetic", default=None,
        help=(
            "Path to a held-out synthetic eval database (never trained on). "
            "After training, evaluates the model on this data and reports "
            "AUC/PR-AUC. Typically ~/Documents/notes-AOSDL/labeler/data/synthetic_eval.sqlite."
        ),
    )
    args = parser.parse_args()
    if args.output is None:
        args.output = DEFAULT_OUTPUT_V2 if args.architecture == "mlp" else DEFAULT_OUTPUT_V1
    # Grokking presets: override epochs/patience/weight-decay if --grok
    if args.grok:
        if args.epochs == 500:   args.epochs = 5000
        if args.patience == 50:  args.patience = 500
        if args.weight_decay is None: args.weight_decay = 0.5
        print(f"[grok] regime: epochs={args.epochs}, patience={args.patience}, "
              f"weight_decay={args.weight_decay}, lr={args.lr}")
    if args.weight_decay is None:
        args.weight_decay = 0.01
    return train(args)


if __name__ == "__main__":
    sys.exit(main())
