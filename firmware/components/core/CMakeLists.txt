idf_component_register(
    SRCS
        "src/action_queue.cpp"
        "src/event_bus.cpp"
        "src/request_orchestrator.cpp"
        "src/service_manager.cpp"
        "src/state_store.cpp"
        "src/ui_dispatcher.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        models
        utils
)
