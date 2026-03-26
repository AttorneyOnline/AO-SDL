/**
 * @file JsonConfiguration.h
 * @brief JSON-backed implementation of ConfigurationBase.
 * @ingroup configuration
 */
#pragma once

#include "configuration/IConfiguration.h"

#include <json.hpp>

#include <any>
#include <cstdint>
#include <string>
#include <vector>

/// CRTP intermediate that implements all ConfigurationBase do_* hooks using
/// a flat nlohmann::json object as the backing store.
///
/// Subclass it with the CRTP pattern:
///
///   class UserConfiguration : public JsonConfiguration<UserConfiguration> {};
///
template <typename Derived>
class JsonConfiguration : public ConfigurationBase<Derived> {
  public:
    static Derived& instance() {
        static_assert(std::is_base_of_v<JsonConfiguration, Derived>, "Derived must inherit from JsonConfiguration");
        return ConfigurationBase<Derived>::instance();
    }

  protected:
    // -- defaults ------------------------------------------------------------

    /// Set a JSON object whose keys serve as fallback values.
    ///
    /// Call this once (typically in the subclass constructor) to define all
    /// known keys and their default values in a single place:
    ///
    ///   set_defaults({
    ///       {"http_port", 8080},
    ///       {"server_name", "My Server"},
    ///   });
    ///
    /// - value() falls through to the default when a key is absent from json_.
    /// - keys() and for_each() include defaulted keys that haven't been set.
    /// - serialize() merges defaults under live values so the output is complete.
    void set_defaults(nlohmann::json defaults) {
        defaults_ = std::move(defaults);
    }

    /// Read-only access to the defaults object.
    const nlohmann::json& defaults() const {
        return defaults_;
    }

    // -- serialization -------------------------------------------------------

    bool do_deserialize(const std::vector<uint8_t>& data) override {
        auto parsed = nlohmann::json::parse(data, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object())
            return false;
        json_ = std::move(parsed);
        return true;
    }

    std::vector<uint8_t> do_serialize() const override {
        // Merge: defaults first, then live values on top.
        auto merged = defaults_;
        merged.merge_patch(json_);
        auto s = merged.dump(4);
        return {s.begin(), s.end()};
    }

    // -- key/value -----------------------------------------------------------
    //
    // Keys that contain '/' are interpreted as JSON Pointer paths:
    //   "servers/0/name"  →  json_pointer("/servers/0/name")
    //
    // Plain keys (no '/') use direct object lookup for efficiency.

    void do_set_value(const std::string& key, const std::any& value) override {
        if (is_path(key))
            json_[to_pointer(key)] = any_to_json(value);
        else
            json_[key] = any_to_json(value);
    }

    std::any do_value(const std::string& key, const std::any& default_value) const override {
        if (is_path(key)) {
            auto ptr = to_pointer(key);
            if (json_.contains(ptr))
                return json_to_any(json_.at(ptr), default_value);
            if (defaults_.contains(ptr))
                return json_to_any(defaults_.at(ptr), default_value);
            return default_value;
        }
        auto it = json_.find(key);
        if (it != json_.end())
            return json_to_any(*it, default_value);
        auto dit = defaults_.find(key);
        if (dit != defaults_.end())
            return json_to_any(*dit, default_value);
        return default_value;
    }

    bool do_contains(const std::string& key) const override {
        if (is_path(key))
            return json_.contains(to_pointer(key)) || defaults_.contains(to_pointer(key));
        return json_.contains(key) || defaults_.contains(key);
    }

    void do_remove(const std::string& key) override {
        if (!is_path(key)) {
            json_.erase(key);
            return;
        }
        auto sep = key.rfind('/');
        auto parent_path = key.substr(0, sep);
        auto child_seg = key.substr(sep + 1);
        // Resolve the parent — could itself be a path or a flat key.
        auto parent_ptr = to_pointer(parent_path);
        if (!json_.contains(parent_ptr))
            return;
        auto& parent = json_.at(parent_ptr);
        if (parent.is_array()) {
            auto idx = std::stoull(child_seg);
            if (idx < parent.size())
                parent.erase(parent.begin() + static_cast<ptrdiff_t>(idx));
        }
        else if (parent.is_object()) {
            parent.erase(child_seg);
        }
    }

    void do_clear() override {
        json_ = nlohmann::json::object();
    }

    std::vector<std::string> do_keys() const override {
        auto merged = defaults_;
        merged.merge_patch(json_);
        std::vector<std::string> result;
        flatten_keys(merged, "", result);
        return result;
    }

    void do_for_each(const IConfiguration::KeyValueVisitor& visitor) const override {
        auto merged = defaults_;
        merged.merge_patch(json_);
        flatten_visit(merged, "", visitor);
    }

    /// Direct access for subclasses that need richer JSON manipulation.
    nlohmann::json& json() {
        return json_;
    }
    const nlohmann::json& json() const {
        return json_;
    }

  private:
    // -- std::any  <-->  nlohmann::json conversion helpers -------------------

    static nlohmann::json any_to_json(const std::any& v) {
        if (!v.has_value())
            return nullptr;
        if (v.type() == typeid(bool))
            return std::any_cast<bool>(v);
        if (v.type() == typeid(int))
            return std::any_cast<int>(v);
        if (v.type() == typeid(unsigned int))
            return std::any_cast<unsigned int>(v);
        if (v.type() == typeid(int64_t))
            return std::any_cast<int64_t>(v);
        if (v.type() == typeid(uint64_t))
            return std::any_cast<uint64_t>(v);
        if (v.type() == typeid(float))
            return std::any_cast<float>(v);
        if (v.type() == typeid(double))
            return std::any_cast<double>(v);
        if (v.type() == typeid(std::string))
            return std::any_cast<std::string>(v);
        if (v.type() == typeid(const char*))
            return std::string(std::any_cast<const char*>(v));
        if (v.type() == typeid(nlohmann::json))
            return std::any_cast<nlohmann::json>(v);
        // Unsupported type — store null so the key still exists.
        return nullptr;
    }

    /// Convert a JSON value back to std::any.  When the caller supplied a
    /// typed default we try to honour that type; otherwise we pick the
    /// natural C++ type for the JSON value.
    static std::any json_to_any(const nlohmann::json& j, const std::any& default_value) {
        if (j.is_null())
            return default_value;

        // If the caller provided a typed default, try to coerce the JSON
        // value into that same C++ type so that value<T>() works naturally.
        if (default_value.has_value())
            return json_to_any_typed(j, default_value);

        // No type hint — use natural mapping.
        if (j.is_boolean())
            return j.get<bool>();
        if (j.is_number_integer())
            return j.get<int64_t>();
        if (j.is_number_unsigned())
            return j.get<uint64_t>();
        if (j.is_number_float())
            return j.get<double>();
        if (j.is_string())
            return j.get<std::string>();
        // Arrays / objects — wrap the json value itself.
        return j;
    }

    /// Attempt to convert @p j to the same C++ type as @p hint.
    static std::any json_to_any_typed(const nlohmann::json& j, const std::any& hint) {
        if (hint.type() == typeid(bool) && j.is_boolean())
            return j.get<bool>();
        if (hint.type() == typeid(int) && j.is_number())
            return j.get<int>();
        if (hint.type() == typeid(unsigned int) && j.is_number())
            return j.get<unsigned int>();
        if (hint.type() == typeid(int64_t) && j.is_number())
            return j.get<int64_t>();
        if (hint.type() == typeid(uint64_t) && j.is_number())
            return j.get<uint64_t>();
        if (hint.type() == typeid(float) && j.is_number())
            return j.get<float>();
        if (hint.type() == typeid(double) && j.is_number())
            return j.get<double>();
        if (hint.type() == typeid(std::string) && j.is_string())
            return j.get<std::string>();
        if (hint.type() == typeid(nlohmann::json))
            return j;
        // Type mismatch — fall back to natural mapping so the value isn't lost.
        return json_to_any(j, std::any{});
    }

    // -- iteration helpers ----------------------------------------------------

    /// Recursively collect leaf-node paths from the JSON tree.
    static void flatten_keys(const nlohmann::json& j, const std::string& prefix, std::vector<std::string>& out) {
        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                std::string child = prefix.empty() ? it.key() : prefix + "/" + it.key();
                if (it->is_object() || it->is_array())
                    flatten_keys(*it, child, out);
                else
                    out.push_back(child);
            }
        }
        else if (j.is_array()) {
            for (size_t i = 0; i < j.size(); ++i) {
                std::string child = prefix + "/" + std::to_string(i);
                if (j[i].is_object() || j[i].is_array())
                    flatten_keys(j[i], child, out);
                else
                    out.push_back(child);
            }
        }
    }

    /// Recursively visit leaf-node key-value pairs.
    static void flatten_visit(const nlohmann::json& j, const std::string& prefix,
                              const IConfiguration::KeyValueVisitor& visitor) {
        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                std::string child = prefix.empty() ? it.key() : prefix + "/" + it.key();
                if (it->is_object() || it->is_array())
                    flatten_visit(*it, child, visitor);
                else
                    visitor(child, json_to_any(*it, {}));
            }
        }
        else if (j.is_array()) {
            for (size_t i = 0; i < j.size(); ++i) {
                std::string child = prefix + "/" + std::to_string(i);
                if (j[i].is_object() || j[i].is_array())
                    flatten_visit(j[i], child, visitor);
                else
                    visitor(child, json_to_any(j[i], {}));
            }
        }
    }

    // -- path helpers --------------------------------------------------------

    /// Returns true when @p key contains '/' and should be treated as a path.
    static bool is_path(const std::string& key) {
        return key.find('/') != std::string::npos;
    }

    /// Convert "a/b/0" into a nlohmann::json_pointer "/a/b/0".
    static nlohmann::json::json_pointer to_pointer(const std::string& key) {
        return nlohmann::json::json_pointer("/" + key);
    }

    nlohmann::json json_ = nlohmann::json::object();
    nlohmann::json defaults_ = nlohmann::json::object();
};
