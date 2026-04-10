#!/usr/bin/env bash
# query-moderation-db.sh — run ad-hoc queries against the moderation_events
# table on the staging instance via SSM.
#
# The moderation_events table is the queryable ground truth for all
# moderation decisions. Grafana metrics give you rate / distribution;
# this script gives you the raw events so you can answer "what did
# this user do, and when?".
#
# Usage:
#   scripts/query-moderation-db.sh                 # recent 20 events
#   scripts/query-moderation-db.sh IPID=abcd1234   # by IPID
#   scripts/query-moderation-db.sh ACTION=mute     # by action
#
# Requires: aws cli with SSM permissions, jq.

set -euo pipefail

STACK="${STACK:-kagami-staging}"
AWS_REGION="${AWS_REGION:-us-west-2}"

LIMIT=20
IPID=""
ACTION=""

for arg in "$@"; do
    case "$arg" in
        LIMIT=*) LIMIT="${arg#LIMIT=}" ;;
        IPID=*)  IPID="${arg#IPID=}" ;;
        ACTION=*) ACTION="${arg#ACTION=}" ;;
    esac
done

# Build the WHERE clause.
where=""
if [ -n "$IPID" ]; then
    where="WHERE IPID = '$IPID'"
fi
if [ -n "$ACTION" ]; then
    if [ -n "$where" ]; then
        where="$where AND ACTION = '$ACTION'"
    else
        where="WHERE ACTION = '$ACTION'"
    fi
fi

# SQLite query. Pretty-print the highest-value columns.
SQL="SELECT
    datetime(TIMESTAMP_MS/1000, 'unixepoch') AS ts,
    IPID,
    CHANNEL,
    ACTION,
    ROUND(HEAT_AFTER, 2) AS heat,
    SUBSTR(MESSAGE_SAMPLE, 1, 40) AS msg,
    REASON
FROM moderation_events
$where
ORDER BY TIMESTAMP_MS DESC
LIMIT $LIMIT;"

INSTANCE_ID=$(aws ec2 describe-instances \
    --region "$AWS_REGION" \
    --filters "Name=tag:aws:cloudformation:stack-name,Values=$STACK" \
              "Name=instance-state-name,Values=running" \
    --query 'Reservations[0].Instances[0].InstanceId' --output text)

if [ -z "$INSTANCE_ID" ] || [ "$INSTANCE_ID" = "None" ]; then
    echo "ERROR: no running instance for stack $STACK in $AWS_REGION"
    exit 1
fi

echo "Querying moderation_events on $INSTANCE_ID..."
CMD=$(printf 'sqlite3 -column -header /opt/kagami/kagami.db "%s"' "$SQL")

aws ssm send-command \
    --region "$AWS_REGION" \
    --instance-ids "$INSTANCE_ID" \
    --document-name AWS-RunShellScript \
    --parameters "commands=[\"$CMD\"]" \
    --query 'Command.CommandId' --output text > /tmp/moderation-query-cmd-id

CMD_ID=$(cat /tmp/moderation-query-cmd-id)
sleep 2

aws ssm get-command-invocation \
    --region "$AWS_REGION" \
    --instance-id "$INSTANCE_ID" \
    --command-id "$CMD_ID" \
    --query 'StandardOutputContent' --output text
