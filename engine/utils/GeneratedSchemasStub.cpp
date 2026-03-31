/// Stub implementation of schema lookup functions used when
/// AOSDL_GENERATE_SCHEMAS is OFF or Python/PyYAML are unavailable.
/// Returns a no-op schema that accepts any input, but logs an
/// ERROR on every call so the operator knows validation is disabled.

#include "utils/GeneratedSchemas.h"
#include "utils/JsonSchema.h"
#include "utils/Log.h"

static bool warned = false;

static void warn_once() {
    if (!warned) {
        Log::log_print(ERR, "Schema validation is DISABLED — built without -DAOSDL_GENERATE_SCHEMAS=ON. "
                            "Request bodies will NOT be validated. Install Python 3 + PyYAML and rebuild.");
        warned = true;
    }
}

const JsonSchema& aonx_request_schema(const std::string& operation_id) {
    static const JsonSchema noop;
    Log::log_print(ERR, "Schema validation SKIPPED for operation \"%s\" — schemas not generated", operation_id.c_str());
    warn_once();
    return noop;
}

const JsonSchema& aonx_component_schema(const std::string& name) {
    static const JsonSchema noop;
    Log::log_print(ERR, "Schema validation SKIPPED for component \"%s\" — schemas not generated", name.c_str());
    warn_once();
    return noop;
}
