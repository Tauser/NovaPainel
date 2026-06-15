idf_component_register(
    SRCS
        "src/event_bus.cpp"
        "src/state_store.cpp"
        "src/service_manager.cpp"
        "src/request_orchestrator.cpp"
        "src/ui_dispatcher.cpp"
    INCLUDE_DIRS "include"
    REQUIRES models utils
)
