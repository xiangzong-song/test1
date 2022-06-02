extern "C"
{
    #include "audio_manager.h"
    #include "software_fft.h"
    extern int audio_beat_value_get(int16_t *sample_data, audio_para_t para);
};
#include "CppUTest/TestHarness.h"
TEST_GROUP(TestCase){
    void setup()
    {
    }
    void teardown()
    {
    }
};
TEST(TestCase, testSuccessCase)
{
    audio_para_t param;
    int16_t sample_data[256]={0};

    int beat_value = audio_beat_value_get(sample_data, param);
    int ret=-1;
    if (beat_value > 0)
        ret=0;

    LONGS_EQUAL(0, ret);
}
#if 0
TEST(TestCase, testFailCase)
{
    LONGS_EQUAL(1, );
}

TEST(SystemManager, RegisterModeForStart)
{
    mode_operator_t mode_operator = {(void (*)(void))1, (void (*)(void))2, (void (*)(void))3, 0};
    LightApp_system_manager_register("test", mode_operator);
    LONGS_EQUAL(1, FakeSystemManager_GetStartByName("test"));
}

TEST(SystemManager, RegisterModeForStop)
{
    mode_operator_t mode_operator = {(void (*)(void))1, (void (*)(void))2, (void (*)(void))3, 0};
    LightApp_system_manager_register("test", mode_operator);
    LONGS_EQUAL(2, FakeSystemManager_GetStopByName("test"));
}
TEST(SystemManager, RegisterModeForProcess)
{
    mode_operator_t mode_operator = {(void (*)(void))1, (void (*)(void))2, (void (*)(void))3, 0};
    LightApp_system_manager_register("test", mode_operator);
    LONGS_EQUAL(3, FakeSystemManager_GetProcessByName("test"));
}
TEST(SystemManager, RegisterSameModeNameAgain)
{
    mode_operator_t mode_operator = {(void (*)(void))1, (void (*)(void))2, (void (*)(void))3, 0};
    LightApp_system_manager_register("test", mode_operator);
    LONGS_EQUAL(3, FakeSystemManager_GetProcessByName("test"));
    mode_operator.process = (void (*)(void))6;
    LightApp_system_manager_register("test", mode_operator);
    LONGS_EQUAL(6, FakeSystemManager_GetProcessByName("test"));
}
TEST(SystemManager, RegisterMultipleModeName)
{
    mode_operator_t mode_testOperator = {(void (*)(void))1, (void (*)(void))2, (void (*)(void))3, 0};
    mode_operator_t mode_basicOperator = {(void (*)(void))6, (void (*)(void))7, (void (*)(void))8, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    LightApp_system_manager_register("basic", mode_basicOperator);
    LONGS_EQUAL(3, FakeSystemManager_GetProcessByName("test"));
    LONGS_EQUAL(8, FakeSystemManager_GetProcessByName("basic"));
}
TEST(SystemManager, UnRegisterNothingModeName)
{
    LightApp_system_manager_unregister("test");
    POINTERS_EQUAL(NULL, FakeSystemManager_GetProcessByName("test")); //
}
TEST(SystemManager, UnregisterSingleModeName)
{
    mode_operator_t mode_testOperator = {(void (*)(void))1, (void (*)(void))2, (void (*)(void))3, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    LONGS_EQUAL(3, FakeSystemManager_GetProcessByName("test"));
    LightApp_system_manager_unregister("test");
    POINTERS_EQUAL(NULL, FakeSystemManager_GetProcessByName("test"));
}
TEST(SystemManager, UnRegisterOneFromMultipleModeName)
{
    mode_operator_t mode_testOperator = {(void (*)(void))1, (void (*)(void))2, (void (*)(void))3, 0};
    mode_operator_t mode_basicOperator = {(void (*)(void))6, (void (*)(void))7, (void (*)(void))8, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    LightApp_system_manager_register("basic", mode_basicOperator);
    LONGS_EQUAL(3, FakeSystemManager_GetProcessByName("test"));
    LONGS_EQUAL(8, FakeSystemManager_GetProcessByName("basic"));
    LightApp_system_manager_unregister("basic");
    POINTERS_EQUAL(NULL, FakeSystemManager_GetProcessByName("basic")); //
}
TEST(SystemManager, nullEvent)
{
    POINTERS_EQUAL(NULL, NULL);
}
TEST(SystemManager, NotifyStartFailNoExistMode)
{
    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_FAIL, result);
}
TEST(SystemManager, NotifyStartSuccessExistNullMode)
{
    mode_operator_t mode_testOperator = {NULL, NULL, NULL, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}
TEST(SystemManager, NotifyStartSuccessExistMode)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(1, state);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NotifyStopSuccessExistMode)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_STOP;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(2, state);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTOP, state);
}
TEST(SystemManager, NotifyStopSuccessExistMultiplyMode)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    mode_operator_t mode_basicOperator = {FakeBasicStart, FakeBasicStop, FakeBasicProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    LightApp_system_manager_register("basic", mode_basicOperator);
    mode_event_e event = MODE_STOP;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(2, state);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTOP, state);

    event = MODE_START;
    result = LightApp_system_manager_notify("basic", event);
    LONGS_EQUAL(1, basic_state);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NotifyStopSuccessNextNullOperator)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    mode_operator_t mode_basicOperator = {NULL, NULL, NULL, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    LightApp_system_manager_register("basic", mode_basicOperator);
    mode_event_e event = MODE_STOP;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(2, state);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t getstate = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTOP, getstate);

    event = MODE_START;
    result = LightApp_system_manager_notify("basic", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    getstate = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, getstate);
}

TEST(SystemManager, NotifyColorSuccessExistMode)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_COLOR;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_TURNON, result);
    result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    // int32_t state=FakeSystemManager_GetTaskState();
    // LONGS_EQUAL(TASKSTOP,state);
}
TEST(SystemManager, NotifyLuminanceSuccessExistMode)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_LUMINANCE;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_TURNON, result);
    result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}
TEST(SystemManager, NotifyLuminanceAndColorSuccessExistMode)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_LUMINANCE;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_TURNON, result);
    event = MODE_COLOR;
    result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NotifySingleTurnOnSuccessExistMode)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_TURNON;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NotifyTurnOnFailWhenModeStartToTurnOn)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);

    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);

    event = MODE_TURNON;
    result = LightApp_system_manager_notify("", event);
    LONGS_EQUAL(NOTIFY_FAIL, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NotifyTurnOnSuccessWhenModeStop)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);

    mode_event_e event = MODE_STOP;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);

    event = MODE_LUMINANCE;
    result = LightApp_system_manager_notify("", event);
    LONGS_EQUAL(NOTIFY_TURNON, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NotifyFailWhenModeStopToTurnOff)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);

    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("test", event);
    event = MODE_STOP;
    result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);

    event = MODE_TURNOFF;
    result = LightApp_system_manager_notify(NULL, event);
    LONGS_EQUAL(NOTIFY_FAIL, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTOP, state);

}

TEST(SystemManager, NotifyTurnOnWhenModeStartToLuminance)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);

    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);

    event = MODE_LUMINANCE;
    result = LightApp_system_manager_notify(NULL, event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NotifyTurnOnWhenModeStopToLuminance)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);

    mode_event_e event = MODE_STOP;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);

    event = MODE_LUMINANCE;
    result = LightApp_system_manager_notify(NULL, event);
    LONGS_EQUAL(NOTIFY_TURNON, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NullNameNotifyTurnOnFailWhenModeStartToTurnOn)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);

    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);

    event = MODE_TURNON;
    result = LightApp_system_manager_notify(NULL, event);
    LONGS_EQUAL(NOTIFY_FAIL, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NotifyDoubleTurnOnFailExistMode)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_TURNON;
    int32_t result = LightApp_system_manager_notify("test", event);
    result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_FAIL, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}
TEST(SystemManager, NotifyDoubleTurnOffFailExistMode)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_TURNOFF;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(NOTIFY_FAIL, result);
    int32_t state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTOP, state);
}

TEST(SystemManager, NotifySuccessModeChanged)
{
    mode_operator_t mode_testOperator = {FakeStart, FakeStop, FakeProcess, 0};
    mode_operator_t mode_basicOperator = {FakeBasicStart, FakeBasicStop, FakeBasicProcess, 0};
    LightApp_system_manager_register("basic", mode_basicOperator);
    LightApp_system_manager_register("test", mode_testOperator);
    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("test", event);
    LONGS_EQUAL(1, state);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t getstate = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, getstate);

    event = MODE_START;
    result = LightApp_system_manager_notify("basic", event);
    LONGS_EQUAL(2, state);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    state = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, state);
}

TEST(SystemManager, NotifyFailWhenOTAOperatorStart)
{
    mode_operator_t mode_testOperator = {NULL, NULL, NULL, 0};
    mode_operator_t mode_basicOperator = {FakeBasicStart, FakeBasicStop, FakeBasicProcess, 0};
    LightApp_system_manager_register("OTA", mode_testOperator);
    LightApp_system_manager_register("basic", mode_basicOperator);
    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("OTA", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t getstate = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, getstate);

    event = MODE_START;
    result = LightApp_system_manager_notify("basic", event);
    LONGS_EQUAL(NOTIFY_FAIL, result);
}

TEST(SystemManager, NotifySuccessWhenOTAOperatorStop)
{
    mode_operator_t mode_testOperator = {NULL, NULL, NULL, 0};
    mode_operator_t mode_basicOperator = {FakeBasicStart, FakeBasicStop, FakeBasicProcess, 0};
    LightApp_system_manager_register("OTA", mode_testOperator);
    LightApp_system_manager_register("basic", mode_basicOperator);
    mode_event_e event = MODE_STOP;
    int32_t result = LightApp_system_manager_notify("OTA", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t getstate = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTOP, getstate);

    event = MODE_START;
    result = LightApp_system_manager_notify("basic", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    getstate = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, getstate);
}

TEST(SystemManager, NotifySuccessWhenOTAOperator)
{
    mode_operator_t mode_testOperator = {NULL, NULL, NULL, 0};
    mode_operator_t mode_basicOperator = {FakeBasicStart, FakeBasicStop, FakeBasicProcess, 0};
    LightApp_system_manager_register("OTA", mode_testOperator);
    LightApp_system_manager_register("basic", mode_basicOperator);

    mode_event_e event = MODE_START;
    int32_t result = LightApp_system_manager_notify("basic", event);
    LONGS_EQUAL(1, basic_state);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    int32_t getstate = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, getstate);

    event = MODE_START;
    result = LightApp_system_manager_notify("OTA", event);
    LONGS_EQUAL(NOTIFY_SUCCESS, result);
    getstate = FakeSystemManager_GetTaskState();
    LONGS_EQUAL(TASKSTART, getstate);
}
#endif