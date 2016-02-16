{% include 'copyright_block.txt' %}
#include "{{v8_class}}.h"

{% for filename in cpp_includes %}
#include "{{filename}}"
{% endfor %}

namespace blink {

{{v8_class}}::{{v8_class}}(v8::Local<v8::Function> callback, ScriptState* scriptState)
    : ActiveDOMCallback(scriptState->executionContext())
    , m_scriptState(scriptState)
{
    m_callback.set(scriptState->isolate(), callback);
}

{{v8_class}}::~{{v8_class}}()
{
}

DEFINE_TRACE({{v8_class}})
{
    {{cpp_class}}::trace(visitor);
    ActiveDOMCallback::trace(visitor);
}

{% for method in methods if not method.is_custom %}
{{method.cpp_type}} {{v8_class}}::{{method.name}}({{method.argument_declarations | join(', ')}})
{
    {% set return_default = 'return true'
           if method.idl_type == 'boolean' else 'return' %}{# void #}
    if (!canInvokeCallback())
        {{return_default}};

    if (!m_scriptState->contextIsValid())
        {{return_default}};

    ScriptState::Scope scope(m_scriptState.get());
    {% if method.call_with_this_handle %}
    v8::Local<v8::Value> thisHandle = thisValue.v8Value();
    if (thisHandle.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        {{return_default}};
    }
    {% endif %}
    {% for argument in method.arguments %}
    v8::Local<v8::Value> {{argument.handle}} = {{argument.cpp_value_to_v8_value}};
    if ({{argument.handle}}.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        {{return_default}};
    }
    {% endfor %}
    {% if method.arguments %}
    v8::Local<v8::Value> argv[] = { {{method.arguments | join(', ', 'handle')}} };
    {% else %}
    {# Empty array initializers are illegal, and don't compile in MSVC. #}
    v8::Local<v8::Value> *argv = 0;
    {% endif %}

    {% set this_handle_parameter = 'thisHandle, ' if method.call_with_this_handle else 'v8::Undefined(m_scriptState->isolate()), ' %}
    {% if method.idl_type == 'boolean' %}
    v8::TryCatch exceptionCatcher(m_scriptState->isolate());
    exceptionCatcher.SetVerbose(true);
    ScriptController::callFunction(m_scriptState->executionContext(), m_callback.newLocal(m_scriptState->isolate()), {{this_handle_parameter}}{{method.arguments | length}}, argv, m_scriptState->isolate());
    return !exceptionCatcher.HasCaught();
    {% else %}{# void #}
    ScriptController::callFunction(m_scriptState->executionContext(), m_callback.newLocal(m_scriptState->isolate()), {{this_handle_parameter}}{{method.arguments | length}}, argv, m_scriptState->isolate());
    {% endif %}
}

{% endfor %}
} // namespace blink
