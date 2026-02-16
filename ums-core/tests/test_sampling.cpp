#include <gtest/gtest.h>
#include <vector>
#include <cstring>

extern "C" {
#include "ums/ums_core.h"
#include "ums/sampling.h"
}

// Mock transmission data storage
struct TransmissionData {
    std::vector<uint8_t> data;
    uint32_t call_count = 0;
};

static TransmissionData g_tx_data;
static uint32_t g_critical_section_count = 0;
static uint32_t g_mock_time_us = 0;

// Mock transmission function
void mock_transmit(const void *pData, uint32_t length) {
    g_tx_data.call_count++;
    const uint8_t *bytes = static_cast<const uint8_t*>(pData);
    g_tx_data.data.assign(bytes, bytes + length);
}

// Mock critical section functions
void mock_enter_critical(void) {
    g_critical_section_count++;
}

void mock_exit_critical(void) {
    // No-op for tests
}

// Mock time provider
uint32_t mock_time_provider(void) {
    return g_mock_time_us;
}

class UMSCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_tx_data.data.clear();
        g_tx_data.call_count = 0;
        g_critical_section_count = 0;
        g_mock_time_us = 0;
        UMSDestroy();
    }

    void TearDown() override {
        UMSDestroy();
    }
};

TEST_F(UMSCoreTest, SetupTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = mock_enter_critical,
        .exit_critical = mock_exit_critical,
        .time_provider = nullptr
    };
    
    bool result = UMSSetup(&config);
    EXPECT_TRUE(result);
    EXPECT_TRUE(UMSIsInitialized());
}

TEST_F(UMSCoreTest, SetupWithTimeProviderTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = mock_enter_critical,
        .exit_critical = mock_exit_critical,
        .time_provider = mock_time_provider
    };
    
    bool result = UMSSetup(&config);
    EXPECT_TRUE(result);
    EXPECT_TRUE(UMSIsInitialized());
}

TEST_F(UMSCoreTest, SetupNullPointerTest) {
    bool result = UMSSetup(nullptr);
    EXPECT_FALSE(result);
}

TEST_F(UMSCoreTest, SetupNullTransmitFunctionTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = nullptr,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    
    bool result = UMSSetup(&config);
    EXPECT_FALSE(result);
}

TEST_F(UMSCoreTest, SetupWithoutCriticalSectionTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    
    bool result = UMSSetup(&config);
    EXPECT_TRUE(result);
}

TEST_F(UMSCoreTest, DoubleInitializationTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = mock_enter_critical,
        .exit_critical = mock_exit_critical,
        .time_provider = nullptr
    };
    
    EXPECT_TRUE(UMSSetup(&config));
    EXPECT_FALSE(UMSSetup(&config)); // Should fail
}

TEST_F(UMSCoreTest, TraceVariableTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    UMSSetup(&config);
    
    float var1 = 1.5f;
    bool result = UMSTrace(&var1);
    EXPECT_TRUE(result);
    EXPECT_EQ(UMSGetChannelCount(), 1);
}

TEST_F(UMSCoreTest, TraceNullPointerTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    UMSSetup(&config);
    
    bool result = UMSTrace(nullptr);
    EXPECT_FALSE(result);
}

TEST_F(UMSCoreTest, TraceMultipleVariablesTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    UMSSetup(&config);
    
    float var1 = 1.0f, var2 = 2.0f, var3 = 3.0f;
    
    EXPECT_TRUE(UMSTrace(&var1));
    EXPECT_TRUE(UMSTrace(&var2));
    EXPECT_TRUE(UMSTrace(&var3));
    EXPECT_EQ(UMSGetChannelCount(), 3);
}

TEST_F(UMSCoreTest, TraceMaxChannelsTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    UMSSetup(&config);
    
    float vars[UMS_MAX_CHANNELS + 1];
    
    for (int i = 0; i < UMS_MAX_CHANNELS; i++) {
        vars[i] = static_cast<float>(i);
        EXPECT_TRUE(UMSTrace(&vars[i]));
    }
    
    // Should fail on max + 1
    EXPECT_FALSE(UMSTrace(&vars[UMS_MAX_CHANNELS]));
    EXPECT_EQ(UMSGetChannelCount(), UMS_MAX_CHANNELS);
}

TEST_F(UMSCoreTest, UpdateWithoutSetupTest) {
    bool result = UMSUpdate();
    EXPECT_FALSE(result);
}

TEST_F(UMSCoreTest, UpdateWithoutTracedVariablesTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    UMSSetup(&config);
    
    bool result = UMSUpdate();
    EXPECT_FALSE(result);
}

TEST_F(UMSCoreTest, UpdateTransmitsDataTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    UMSSetup(&config);
    
    float var1 = 42.5f;
    UMSTrace(&var1);
    
    bool result = UMSUpdate();
    EXPECT_TRUE(result);
    EXPECT_EQ(g_tx_data.call_count, 1);
    
    // Verify transmitted data
    ASSERT_GE(g_tx_data.data.size(), sizeof(uint32_t) + sizeof(uint8_t) + sizeof(float));
    
    ums_sample_t *sample = reinterpret_cast<ums_sample_t*>(g_tx_data.data.data());
    EXPECT_EQ(sample->channels, 1);
    EXPECT_FLOAT_EQ(sample->values[0], 42.5f);
}

TEST_F(UMSCoreTest, CriticalSectionCalledTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = mock_enter_critical,
        .exit_critical = mock_exit_critical,
        .time_provider = nullptr
    };
    UMSSetup(&config);
    
    float var1 = 1.0f;
    UMSTrace(&var1);
    
    g_critical_section_count = 0;
    UMSUpdate();
    
    // Should enter critical section at least twice (timestamp and buffer swap)
    EXPECT_GE(g_critical_section_count, 2);
}

TEST_F(UMSCoreTest, TripleBufferingTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    UMSSetup(&config);
    
    float var1 = 1.0f;
    UMSTrace(&var1);
    
    // First update - should trigger transmission
    UMSUpdate();
    EXPECT_EQ(g_tx_data.call_count, 1);
    float first_value = reinterpret_cast<ums_sample_t*>(g_tx_data.data.data())->values[0];
    EXPECT_FLOAT_EQ(first_value, 1.0f);
    
    // Second update while transmitting - should queue
    var1 = 2.0f;
    UMSUpdate();
    EXPECT_EQ(g_tx_data.call_count, 1); // Still transmitting first
    
    // Complete transmission - should trigger second
    UMSTransmissionComplete();
    EXPECT_EQ(g_tx_data.call_count, 2);
    float second_value = reinterpret_cast<ums_sample_t*>(g_tx_data.data.data())->values[0];
    EXPECT_FLOAT_EQ(second_value, 2.0f);
}

TEST_F(UMSCoreTest, CounterTimestampIncrementTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr  // Use counter mode
    };
    UMSSetup(&config);
    
    float var1 = 1.0f;
    UMSTrace(&var1);
    
    // First update - starts transmission immediately
    UMSUpdate();
    EXPECT_EQ(g_tx_data.call_count, 1);
    ums_sample_t sample1;
    memcpy(&sample1, g_tx_data.data.data(), sizeof(ums_sample_t));
    EXPECT_EQ(sample1.timestamp, 0);
    
    // Second update while first transmission is active - queues data
    UMSUpdate();
    EXPECT_EQ(g_tx_data.call_count, 1); // Still transmitting first
    
    // Complete first transmission - should start transmitting the queued second update
    UMSTransmissionComplete();
    EXPECT_EQ(g_tx_data.call_count, 2);
    ums_sample_t sample2;
    memcpy(&sample2, g_tx_data.data.data(), sizeof(ums_sample_t));
    EXPECT_EQ(sample2.timestamp, 1);
    
    // Third update while second transmission is active
    UMSUpdate();
    EXPECT_EQ(g_tx_data.call_count, 2); // Still transmitting second
    
    // Complete second transmission - should start transmitting the queued third update
    UMSTransmissionComplete();
    EXPECT_EQ(g_tx_data.call_count, 3);
    ums_sample_t sample3;
    memcpy(&sample3, g_tx_data.data.data(), sizeof(ums_sample_t));
    EXPECT_EQ(sample3.timestamp, 2);
}

TEST_F(UMSCoreTest, TimeProviderTimestampTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = mock_time_provider  // Use time provider mode
    };
    UMSSetup(&config);
    
    float var1 = 1.0f;
    UMSTrace(&var1);
    
    // First sample at 1000 microseconds
    g_mock_time_us = 1000;
    UMSUpdate();
    EXPECT_EQ(g_tx_data.call_count, 1);
    ums_sample_t sample1;
    memcpy(&sample1, g_tx_data.data.data(), sizeof(ums_sample_t));
    EXPECT_EQ(sample1.timestamp, 1000);
    
    // Second sample at 2500 microseconds (1.5ms later)
    g_mock_time_us = 2500;
    UMSUpdate();
    
    UMSTransmissionComplete();
    EXPECT_EQ(g_tx_data.call_count, 2);
    ums_sample_t sample2;
    memcpy(&sample2, g_tx_data.data.data(), sizeof(ums_sample_t));
    EXPECT_EQ(sample2.timestamp, 2500);
    
    // Third sample at 5000 microseconds (2.5ms later)
    g_mock_time_us = 5000;
    UMSUpdate();
    
    UMSTransmissionComplete();
    EXPECT_EQ(g_tx_data.call_count, 3);
    ums_sample_t sample3;
    memcpy(&sample3, g_tx_data.data.data(), sizeof(ums_sample_t));
    EXPECT_EQ(sample3.timestamp, 5000);
}

TEST_F(UMSCoreTest, TimeProviderIrregularSamplingTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = mock_time_provider
    };
    UMSSetup(&config);
    
    float var1 = 1.0f;
    UMSTrace(&var1);
    
    // Simulate irregular sampling intervals
    uint32_t timestamps[] = {0, 100, 250, 1000, 1001, 5000};
    
    for (size_t i = 0; i < sizeof(timestamps) / sizeof(timestamps[0]); i++) {
        g_mock_time_us = timestamps[i];
        UMSUpdate();
        
        if (i > 0) {
            UMSTransmissionComplete();
        }
        
        ums_sample_t sample;
        memcpy(&sample, g_tx_data.data.data(), sizeof(ums_sample_t));
        EXPECT_EQ(sample.timestamp, timestamps[i]);
    }
}

TEST_F(UMSCoreTest, TimeProviderWithCriticalSectionTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = mock_enter_critical,
        .exit_critical = mock_exit_critical,
        .time_provider = mock_time_provider
    };
    UMSSetup(&config);
    
    float var1 = 1.0f;
    UMSTrace(&var1);
    
    g_mock_time_us = 12345;
    g_critical_section_count = 0;
    
    UMSUpdate();
    
    // With time provider, should only enter critical section once (for buffer swap)
    // No critical section needed for timestamp reading
    EXPECT_GE(g_critical_section_count, 1);
    
    ums_sample_t sample;
    memcpy(&sample, g_tx_data.data.data(), sizeof(ums_sample_t));
    EXPECT_EQ(sample.timestamp, 12345);
}

TEST_F(UMSCoreTest, SampleSizeCalculationTest) {
    EXPECT_EQ(ums_sample_size(1), 9);  // 4 + 1 + 4
    EXPECT_EQ(ums_sample_size(3), 17); // 4 + 1 + 12
    EXPECT_EQ(ums_sample_size(6), 29); // 4 + 1 + 24
}

TEST_F(UMSCoreTest, ConstCorrectnessTest) {
    ums_core_transmission_config_t config = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    UMSSetup(&config);
    
    const float const_var = 99.9f;
    bool result = UMSTrace(&const_var);
    EXPECT_TRUE(result);
}

TEST_F(UMSCoreTest, MixedModeOperationTest) {
    // Test switching between counter and time provider modes
    // (requires re-initialization)
    
    // First, use counter mode
    ums_core_transmission_config_t config1 = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = nullptr
    };
    UMSSetup(&config1);
    
    float var1 = 1.0f;
    UMSTrace(&var1);
    UMSUpdate();
    
    ums_sample_t sample1;
    memcpy(&sample1, g_tx_data.data.data(), sizeof(ums_sample_t));
    EXPECT_EQ(sample1.timestamp, 0); // Counter starts at 0
    
    // Destroy and reinitialize with time provider
    UMSDestroy();
    
    ums_core_transmission_config_t config2 = {
        .transmit_fn = mock_transmit,
        .enter_critical = nullptr,
        .exit_critical = nullptr,
        .time_provider = mock_time_provider
    };
    UMSSetup(&config2);
    
    UMSTrace(&var1);
    g_mock_time_us = 99999;
    UMSUpdate();
    
    ums_sample_t sample2;
    memcpy(&sample2, g_tx_data.data.data(), sizeof(ums_sample_t));
    EXPECT_EQ(sample2.timestamp, 99999); // Now using time provider
}