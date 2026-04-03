/**
 * @file PrometheusFormatter.h
 * @brief Renders metrics in Prometheus text exposition format (v0.0.4).
 *
 * Output is suitable for serving on a /metrics HTTP endpoint that
 * Prometheus scrapes at regular intervals.
 */
#pragma once

#include "metrics/Metrics.h"

#include <sstream>
#include <string>

namespace metrics {

class PrometheusFormatter {
  public:
    /// Render a single metric family to Prometheus text format.
    void format(const MetricFamilyBase& family, std::string& out) const {
        out += "# HELP " + family.name() + " " + family.help() + "\n";
        out += "# TYPE " + family.name() + " " + family.type_string() + "\n";

        family.visit([&](const std::vector<std::string>& label_values, double value) {
            out += family.name();

            // Append labels if present
            auto& label_names = family.label_names();
            if (!label_names.empty() && !label_values.empty()) {
                out += '{';
                for (size_t i = 0; i < label_names.size() && i < label_values.size(); ++i) {
                    if (i > 0)
                        out += ',';
                    out += label_names[i] + "=\"" + escape_label_value(label_values[i]) + "\"";
                }
                out += '}';
            }

            out += ' ';
            out += format_value(value);
            out += '\n';
        });

        out += '\n';
    }

  private:
    /// Escape backslash, double-quote, and newline in label values.
    static std::string escape_label_value(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '\\')
                out += "\\\\";
            else if (c == '"')
                out += "\\\"";
            else if (c == '\n')
                out += "\\n";
            else
                out += c;
        }
        return out;
    }

    /// Format a double value. Integers are printed without decimal point.
    static std::string format_value(double v) {
        if (std::isinf(v))
            return v > 0 ? "+Inf" : "-Inf";
        if (std::isnan(v))
            return "NaN";
        // If it's a whole number, print without decimals
        if (v == static_cast<double>(static_cast<uint64_t>(v)) && v < 1e15)
            return std::to_string(static_cast<uint64_t>(v));
        // Otherwise use enough precision
        std::ostringstream ss;
        ss << v;
        return ss.str();
    }
};

} // namespace metrics
