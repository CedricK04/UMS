//
//
//

#include <gtest/gtest.h>
#include <array>
#include <cstring>
#include <string>

#include "ums/ums_core.h"
#include "ums/triple_buffer.h"

// ── Globals exposed for white-box inspection ──────────────────────────────────
// Must remain plain C extern declarations to match library linkage.

extern sample_packet_t  ums_triple_buffer[3];
extern volatile uint8_t idx_write;
extern volatile uint8_t idx_read;
extern volatile uint8_t idx_spare;
extern uint8_t          channel_count;
extern bool             g_ums_initialized;
extern uint16_t         g_actual_frame_size;
extern volatile bool    g_dma_busy;
extern data_channel_t   registry[UMS_MAX_CHANNELS];

// ── Platform hook overrides ───────────────────────────────────────────────────
// Strong definitions that override the weak no-ops in ums_core.c.
// Timestamp is controlled via the test fixture to avoid mutable globals.

static uint32_t s_mock_timestamp = 0U;

// Platform functions are intentionally empty for testing - they override weak symbols
void ums_platform_enter_critical(void) { /* No-op override for testing */ }
void ums_platform_exit_critical(void)  { /* No-op override for testing */ }
uint32_t ums_platform_get_timestamp(void) { return s_mock_timestamp; }

// ── Test fixture ──────────────────────────────────────────────────────────────
// Mutable test state lives in the fixture, not at file scope,
// satisfying cpp:S5421 (global variables should be const).

class UmsCoreTest : public ::testing::Test
{
public:
    // Changed from protected to public to avoid cpp:S3656 (member variables should not be protected)
    const void* m_last_tx_ptr    = nullptr;
    uint16_t    m_last_tx_length = 0U;
    uint32_t    m_tx_call_count  = 0U;

    static void transmit_trampoline(void* data_ptr, uint16_t length)
    {
        if (s_current_fixture != nullptr)
        {
            s_current_fixture->m_last_tx_ptr    = data_ptr;
            s_current_fixture->m_last_tx_length = length;
            s_current_fixture->m_tx_call_count++;
        }
    }

    static UmsCoreTest* s_current_fixture;

protected:
    void SetUp() override
    {
        m_last_tx_ptr    = nullptr;
        m_last_tx_length = 0U;
        m_tx_call_count  = 0U;
        s_mock_timestamp = 0U;

        ums_setup([](void* data_ptr, uint16_t length) {
            // Cannot capture 'this' in a plain function pointer —
            // use file-scope accessors via the fixture's static helpers.
            (void)data_ptr;
            (void)length;
        });

        // Re-setup with a capturing trampoline via a static pointer to fixture.
        s_current_fixture = this;
        ums_setup(&UmsCoreTest::transmit_trampoline);
    }

    void TearDown() override
    {
        s_current_fixture = nullptr;
        ums_destroy();
    }
};

UmsCoreTest* UmsCoreTest::s_current_fixture = nullptr;

// ── ums_setup() ───────────────────────────────────────────────────────────────

TEST(UmsSetupTest, NullFunctionPointerReturnsError)
{
    EXPECT_EQ(ums_setup(nullptr), UMS_NULL_POINTER);
}

TEST(UmsSetupTest, ValidFunctionPointerSucceeds)
{
    EXPECT_EQ(ums_setup(UmsCoreTest::transmit_trampoline), UMS_SUCCESS);
    ums_destroy();
}

TEST(UmsSetupTest, SetsInitializedFlag)
{
    ums_setup(UmsCoreTest::transmit_trampoline);
    EXPECT_TRUE(g_ums_initialized);
    ums_destroy();
}

// ── ums_trace() ───────────────────────────────────────────────────────────────

TEST_F(UmsCoreTest, TraceNullVariablePointerReturnsError)
{
    const std::string name = "var";
    EXPECT_EQ(ums_trace(nullptr, name.c_str(), UMS_UINT8), UMS_INVALID_VARIABLE_REGISTRATION);
}

TEST_F(UmsCoreTest, TraceNullNamePointerReturnsError)
{
    uint8_t var = 0U;
    EXPECT_EQ(ums_trace(&var, nullptr, UMS_UINT8), UMS_NULL_POINTER);
}

TEST_F(UmsCoreTest, TraceStringTypeReturnsInvalidParameter)
{
    uint8_t var = 0U;
    const std::string name = "var";
    EXPECT_EQ(ums_trace(&var, name.c_str(), UMS_STRING), UMS_INVALID_PARAMETER);
}

TEST_F(UmsCoreTest, TraceBeforeSetupReturnsNotInitialized)
{
    ums_destroy();
    uint8_t var = 0U;
    const std::string name = "var";
    EXPECT_EQ(ums_trace(&var, name.c_str(), UMS_UINT8), UMS_NOT_INITIALIZED);
}

TEST_F(UmsCoreTest, TraceIncreasesChannelCount)
{
    uint8_t var = 42U;
    const std::string name = "var";
    EXPECT_EQ(channel_count, 0U);
    ums_trace(&var, name.c_str(), UMS_UINT8);
    EXPECT_EQ(channel_count, 1U);
}

TEST_F(UmsCoreTest, TraceAccumulatesFrameSize)
{
    uint8_t  u8  = 0U;
    uint32_t u32 = 0U;
    float    f32 = 0.0F;
    const std::string n1 = "a";
    const std::string n2 = "b";
    const std::string n3 = "c";

    const uint16_t base = g_actual_frame_size;
    ums_trace(&u8,  n1.c_str(), UMS_UINT8);
    ums_trace(&u32, n2.c_str(), UMS_UINT32);
    ums_trace(&f32, n3.c_str(), UMS_FLOAT32);

    EXPECT_EQ(g_actual_frame_size, base + 1U + 4U + 4U);
}

TEST_F(UmsCoreTest, TraceAtMaxChannelsReturnsRangeError)
{
    std::array<uint8_t, UMS_MAX_CHANNELS + 1U> vars{};
    const std::string name = "v";

    for (size_t i = 0; i < UMS_MAX_CHANNELS; i++) {
        EXPECT_EQ(ums_trace(&vars[i], name.c_str(), UMS_UINT8), UMS_SUCCESS);
    }
    EXPECT_EQ(ums_trace(&vars[UMS_MAX_CHANNELS], name.c_str(), UMS_UINT8), UMS_RANGE_ERROR);
}

TEST_F(UmsCoreTest, TraceStoresRegistryEntry)
{
    uint32_t var = 0xDEADBEEFU;
    const std::string name = "sensor";
    ums_trace(&var, name.c_str(), UMS_UINT32);

    EXPECT_EQ(registry[0].var_ptr,  &var);
    EXPECT_EQ(registry[0].var_type, UMS_UINT32);
    EXPECT_STREQ(registry[0].var_name_ptr, name.c_str());
}

// ── ums_update() / ums_create_sample() ───────────────────────────────────────

TEST_F(UmsCoreTest, UpdateWithNoChannelsReturnsRangeError)
{
    EXPECT_EQ(ums_update(), UMS_RANGE_ERROR);
}

TEST_F(UmsCoreTest, UpdateBeforeSetupReturnsNotInitialized)
{
    ums_destroy();
    EXPECT_EQ(ums_update(), UMS_NOT_INITIALIZED);
}

TEST_F(UmsCoreTest, UpdateCallsTransmit)
{
    uint8_t var = 7U;
    const std::string name = "x";
    ums_trace(&var, name.c_str(), UMS_UINT8);

    EXPECT_EQ(ums_update(), UMS_SUCCESS);
    EXPECT_EQ(m_tx_call_count, 1U);
}

TEST_F(UmsCoreTest, UpdateTransmitsExactFrameSize)
{
    uint8_t  u8  = 0U;
    uint32_t u32 = 0U;
    const std::string n1 = "a";
    const std::string n2 = "b";

    ums_trace(&u8,  n1.c_str(), UMS_UINT8);
    ums_trace(&u32, n2.c_str(), UMS_UINT32);

    ums_update();
    EXPECT_EQ(m_last_tx_length, g_actual_frame_size);
}

TEST_F(UmsCoreTest, UpdateSerializesUint8ValueCorrectly)
{
    uint8_t var = 0xABU;
    const std::string name = "val";
    ums_trace(&var, name.c_str(), UMS_UINT8);
    ums_update();

    EXPECT_EQ(ums_triple_buffer[idx_spare].data[0], 0xABU);
}

TEST_F(UmsCoreTest, UpdateSerializesFloat64ValueCorrectly)
{
    double var = 3.14159265358979;
    const std::string name = "pi";
    ums_trace(&var, name.c_str(), UMS_FLOAT64);
    ums_update();

    double result = 0.0;
    memcpy(&result, ums_triple_buffer[idx_spare].data, sizeof(double));
    EXPECT_DOUBLE_EQ(result, var);
}

TEST_F(UmsCoreTest, UpdateSerializesMultipleChannelsInOrder)
{
    uint8_t  u8  = 0x11U;
    uint16_t u16 = 0x2233U;
    const std::string n1 = "a";
    const std::string n2 = "b";

    ums_trace(&u8,  n1.c_str(), UMS_UINT8);
    ums_trace(&u16, n2.c_str(), UMS_UINT16);
    ums_update();

    const uint8_t* const data = ums_triple_buffer[idx_spare].data;
    EXPECT_EQ(data[0], 0x11U);

    uint16_t reconstructed = 0U;
    memcpy(&reconstructed, &data[1], sizeof(uint16_t));
    EXPECT_EQ(reconstructed, 0x2233U);
}

TEST_F(UmsCoreTest, UpdateWritesTimestampFromPlatformHook)
{
    s_mock_timestamp = 0xCAFEBABEU;
    uint8_t var = 0U;
    const std::string name = "t";
    ums_trace(&var, name.c_str(), UMS_UINT8);
    ums_update();

    EXPECT_EQ(ums_triple_buffer[idx_spare].timestamp, 0xCAFEBABEU);
}

TEST_F(UmsCoreTest, UpdateReflectsLiveVariableValue)
{
    uint32_t var = 100U;
    const std::string name = "live";
    ums_trace(&var, name.c_str(), UMS_UINT32);

    ums_update();
    ums_transfer_complete_callback();

    var = 200U;
    ums_update();

    uint32_t result = 0U;
    memcpy(&result, ums_triple_buffer[idx_spare].data, sizeof(uint32_t));
    EXPECT_EQ(result, 200U);
}

// ── DMA busy guard ────────────────────────────────────────────────────────────

TEST_F(UmsCoreTest, UpdateWhileDmaBusyReturnsBufferFull)
{
    uint8_t var = 1U;
    const std::string name = "x";
    ums_trace(&var, name.c_str(), UMS_UINT8);

    ums_update();
    EXPECT_EQ(ums_update(), UMS_BUFFER_FULL);
}

TEST_F(UmsCoreTest, UpdateSucceedsAfterTransferComplete)
{
    uint8_t var = 1U;
    const std::string name = "x";
    ums_trace(&var, name.c_str(), UMS_UINT8);

    ums_update();
    ums_transfer_complete_callback();
    EXPECT_EQ(ums_update(), UMS_SUCCESS);
}

// ── Triple buffer index rotation ──────────────────────────────────────────────

TEST_F(UmsCoreTest, TransferCallbackSwapsSpareAndRead)
{
    const uint8_t spare_before = idx_spare;
    const uint8_t read_before  = idx_read;

    ums_transfer_complete_callback();

    EXPECT_EQ(idx_spare, read_before);
    EXPECT_EQ(idx_read,  spare_before);
}

TEST_F(UmsCoreTest, WriteAndReadIndicesNeverCollide)
{
    uint8_t var = 0U;
    const std::string name = "x";
    ums_trace(&var, name.c_str(), UMS_UINT8);

    for (int i = 0; i < 6; i++) {
        if (!g_dma_busy) {
            ums_update();
        }
        ums_transfer_complete_callback();
        EXPECT_NE(idx_write, idx_read);
        EXPECT_NE(idx_write, idx_spare);
    }
}

// ── ums_destroy() ─────────────────────────────────────────────────────────────

TEST_F(UmsCoreTest, DestroyResetsChannelCount)
{
    uint8_t var = 0U;
    const std::string name = "x";
    ums_trace(&var, name.c_str(), UMS_UINT8);
    ums_destroy();
    EXPECT_EQ(channel_count, 0U);
}

TEST_F(UmsCoreTest, DestroyResetsInitializedFlag)
{
    ums_destroy();
    EXPECT_FALSE(g_ums_initialized);
}

TEST_F(UmsCoreTest, DestroyResetsFrameSize)
{
    uint8_t var = 0U;
    const std::string name = "x";
    ums_trace(&var, name.c_str(), UMS_UINT8);
    ums_destroy();
    EXPECT_EQ(g_actual_frame_size, sizeof(uint32_t));
}

TEST_F(UmsCoreTest, DestroyResetsDmaBusyFlag)
{
    uint8_t var = 0U;
    const std::string name = "x";
    ums_trace(&var, name.c_str(), UMS_UINT8);
    ums_update();
    EXPECT_TRUE(g_dma_busy);
    ums_destroy();
    EXPECT_FALSE(g_dma_busy);
}

TEST_F(UmsCoreTest, DestroyResetsBufferIndices)
{
    uint8_t var = 0U;
    const std::string name = "x";
    ums_trace(&var, name.c_str(), UMS_UINT8);
    ums_update();
    ums_transfer_complete_callback();
    ums_destroy();

    EXPECT_EQ(idx_write, 0U);
    EXPECT_EQ(idx_read,  1U);
    EXPECT_EQ(idx_spare, 2U);
}

TEST_F(UmsCoreTest, DestroyBeforeSetupReturnsNotInitialized)
{
    ums_destroy();
    EXPECT_EQ(ums_destroy(), UMS_NOT_INITIALIZED);
}

TEST_F(UmsCoreTest, ReinitializeAfterDestroySucceeds)
{
    uint8_t var = 42U;
    const std::string name = "x";

    ums_trace(&var, name.c_str(), UMS_UINT8);
    ums_destroy();

    EXPECT_EQ(ums_setup(UmsCoreTest::transmit_trampoline), UMS_SUCCESS);
    EXPECT_EQ(ums_trace(&var, name.c_str(), UMS_UINT8), UMS_SUCCESS);
    EXPECT_EQ(ums_update(), UMS_SUCCESS);
}