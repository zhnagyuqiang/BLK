#include "BLK_API.h"

void OleBLK::checkError()
{
    Blk360G2_Error error = Blk360G2_Api_GetLastError();

    if (error.code != Blk360G2_Error_Ok)
    {
        std::cerr << error.message << std::endl;

        cleanup();

        exit(EXIT_FAILURE);
    }
}

void OleBLK::cleanup()
{
    Blk360G2_DeviceConfigWorkflow_Release(deviceConfigWorkflow);
    Blk360G2_Session_Release(session);
    Blk360G2_Api_Release();
}

int OleBLK::ConnectBLK(const char* ip)
{
    Blk360G2_Api_New(BLK360G2_LIBRARY_VERSION);
    checkError();

    session = Blk360G2_Session_New_Default(ip);
    checkError();

    queue = Blk360G2_EventQueue_New();
    checkError();

    return EXIT_SUCCESS;
}

BlkConfig OleBLK::GetDeviveInfo()
{
    deviceConfigWorkflow = Blk360G2_DeviceConfigWorkflow_Create(session);
    checkError();

    const auto deviceInfo = Blk360G2_DeviceConfigWorkflow_GetDeviceInfo(deviceConfigWorkflow);
    checkError();

    const auto deviceStatus = Blk360G2_DeviceConfigWorkflow_GetDeviceStatus(deviceConfigWorkflow);
    checkError();

    return BlkConfig(deviceInfo, deviceStatus);
}

void OleBLK::DoScan()
{
    measurementWorkflow = Blk360G2_MeasurementWorkflow_Create(session);
    checkError();
    const auto errorSubscription = Blk360G2_MeasurementWorkflow_OnError(measurementWorkflow, queue);
    checkError();
    const auto progressSubscription = Blk360G2_MeasurementWorkflow_OnMeasurementProgress(measurementWorkflow, queue);
    checkError();
    auto parameters = Blk360G2_MeasurementParameters_New();
    parameters.scanConfig.density = Blk360G2_PointCloudDensity_Ultralow;
    parameters.enablePointCloud = true;
    parameters.enableImages = false;
    parameters.visConfig.visMode = Blk360G2_VisMode_Disabled;

    Blk360G2_MeasurementWorkflow_Start(measurementWorkflow, parameters, nullptr);
    checkError();

    std::cout << "Started measurement" << std::endl;
    common::print(parameters);

    while (Blk360G2_EventQueue_Wait(queue, 15000))
    {
        checkError();

        if (Blk360G2_EventQueue_IsEmpty(queue))
        {
            checkError();
            std::cout << "Event queue reached timeout without an event" << std::endl;
            break;
        }

        const auto event = Blk360G2_EventQueue_Pop(queue);
        checkError();

        if (event.sender.handle == errorSubscription.handle)
        {
            common::print(event.error);
            break;
        }
        else if (event.sender.handle == progressSubscription.handle)
        {
            common::print(event.measurementProgress);
            if (event.measurementProgress.progress == 100u)
            {
                std::cout << "Measurement finished successfully!" << std::endl;
                std::cout << "New setup UUID: " << Blk360G2_UUID_Serialize(event.measurementProgress.setupUuid).uuid << std::endl;
                break;
            }
        }
        else
        {
            std::cerr << "Received an unexpected event" << std::endl;
            break;
        }
    }
}

void OleBLK::manipulateSetupMetadata(const Blk360G2_SetupHandle& setupHandle, Blk360G2_SetupMetadata& metadata)
{
    for (std::size_t i = 0; i < std::size(metadata.pose.elements); i++)
    {
        metadata.pose.elements[i] = static_cast<double>(i);
    }

    Blk360G2_DataManipulationWorkflow_Setup_Update(dataManipulationWorkflow, setupHandle, metadata);
    checkError();
    Blk360G2_DataManipulationWorkflow_Setup_SetName(dataManipulationWorkflow, setupHandle, "Example setup name");
    checkError();
    Blk360G2_DataManipulationWorkflow_Setup_SetDescription(dataManipulationWorkflow, setupHandle, "Example setup description");
    checkError();
    Blk360G2_DataManipulationWorkflow_Setup_SetLocation(dataManipulationWorkflow, setupHandle, "Example setup location");
    checkError();
}

void OleBLK::listSetups()
{
    bool firstSetupAltered = false;

    setupsEnumerator = Blk360G2_DataManipulationWorkflow_ListSetups(dataManipulationWorkflow);
    checkError();

    while (Blk360G2_SetupEnumerator_MoveNext(setupsEnumerator))
    {
        checkError();
        const auto setupHandle = Blk360G2_SetupEnumerator_GetCurrent(setupsEnumerator);
        checkError();
        auto metadata = Blk360G2_Setup_GetMetadata(setupHandle);
        checkError();

        if (!firstSetupAltered)
        {
            //manipulateSetupMetadata(setupHandle, metadata);
            firstSetupAltered = true;
        }

        metadata = Blk360G2_Setup_GetMetadata(setupHandle);
        checkError();

        common::print(metadata);

        Blk360G2_Setup_Release(setupHandle);
        checkError();
    }
}

Blk360G2_UUID OleBLK::doScan()
{
    auto parameters = Blk360G2_MeasurementParameters_New();
    parameters.scanConfig.density = Blk360G2_PointCloudDensity_Ultralow;
    parameters.enablePointCloud = true;
    parameters.enableImages = false;
    parameters.visConfig.visMode = Blk360G2_VisMode_Disabled;

    const auto errorSubscription = Blk360G2_MeasurementWorkflow_OnError(measurementWorkflow, eventQueue);
    checkError();
    const auto progressSubscription = Blk360G2_MeasurementWorkflow_OnMeasurementProgress(measurementWorkflow, eventQueue);
    checkError();
    const auto setupStartedSubscription = Blk360G2_MeasurementWorkflow_OnSetupStarted(measurementWorkflow, eventQueue);
    checkError();

    Blk360G2_MeasurementWorkflow_Start(measurementWorkflow, parameters, nullptr);
    checkError();
    std::cout << "Started measurement" << std::endl;

    Blk360G2_UUID newSetupUuid{};

    while (Blk360G2_EventQueue_Wait(eventQueue, 10000))
    {
        checkError();

        if (Blk360G2_EventQueue_IsEmpty(eventQueue))
        {
            checkError();
            std::cout << "Event queue reached timeout without an event" << std::endl;
            break;
        }

        const auto event = Blk360G2_EventQueue_Pop(eventQueue);
        checkError();

        if (event.sender.handle == setupStartedSubscription.handle)
        {
            newSetupUuid = event.setupStarted.setupUuid;
            std::cout << "New setup uuid: " << Blk360G2_UUID_Serialize(newSetupUuid).uuid << std::endl;
        }
        else if (event.sender.handle == errorSubscription.handle)
        {
            std::cerr << "Measurement could not finish: " << event.error.message << std::endl;
            break;
        }
        else if (event.sender.handle == progressSubscription.handle)
        {
            std::cout << "Measurement progress: " << event.measurementProgress.progress << "%" << std::endl;
            if (event.measurementProgress.progress == 100u)
            {
                std::cout << "Measurement finished successfully!" << std::endl;
                break;
            }
        }
        else
        {
            std::cerr << "Received an unexpected event" << std::endl;
            break;
        }
    }
    return newSetupUuid;
}
