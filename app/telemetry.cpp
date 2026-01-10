#include "telemetry.h"

#include <opentelemetry/exporters/ostream/span_exporter_factory.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/provider.h>

namespace ion::app {

Telemetry::Telemetry(const std::string& service_name) {
    auto exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create();
    auto processor =
        opentelemetry::sdk::trace::SimpleSpanProcessorFactory::Create(std::move(exporter));
    auto resource =
        opentelemetry::sdk::resource::Resource::Create({{"service.name", service_name}});
    auto provider = opentelemetry::sdk::trace::TracerProviderFactory::Create(std::move(processor),
                                                                             std::move(resource));

    std::shared_ptr<opentelemetry::trace::TracerProvider> api_provider(std::move(provider));
    opentelemetry::trace::Provider::SetTracerProvider(api_provider);
}

Telemetry::~Telemetry() {
    auto provider = std::static_pointer_cast<opentelemetry::trace::TracerProvider>(
        std::make_shared<opentelemetry::trace::NoopTracerProvider>());
    opentelemetry::trace::Provider::SetTracerProvider(provider);
}

}  // namespace ion::app
