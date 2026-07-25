#include <gtest/gtest.h>
#include "db_impl.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace minidb;

class MiniDBTest : public ::testing::Test {
protected:
    std::string db_path = "test_db_dir";
    void SetUp() override {
        std::filesystem::remove_all(db_path);
    }
    void TearDown() override {
        std::filesystem::remove_all(db_path);
    }
};

TEST_F(MiniDBTest, PutAndGet) {
    DBImpl db(db_path);
    db.Put("key1", "value1");
    db.Put("key2", "value2");
    
    auto v1 = db.Get("key1");
    EXPECT_TRUE(v1.has_value());
    EXPECT_EQ(v1.value(), "value1");

    auto v2 = db.Get("key2");
    EXPECT_TRUE(v2.has_value());
    EXPECT_EQ(v2.value(), "value2");
}

TEST_F(MiniDBTest, UpdateAndDelete) {
    DBImpl db(db_path);
    db.Put("key1", "value1");
    db.Put("key1", "value2");
    
    auto v1 = db.Get("key1");
    EXPECT_TRUE(v1.has_value());
    EXPECT_EQ(v1.value(), "value2");

    db.Delete("key1");
    auto v2 = db.Get("key1");
    EXPECT_FALSE(v2.has_value());
}

TEST_F(MiniDBTest, FlushAndReadFromDisk) {
    {
        DBImpl db(db_path);
        // Force flush by adding a large value > 4MB
        std::string large_val(5 * 1024 * 1024, 'A');
        db.Put("large_key", large_val);
        db.Put("small_key", "small_val");
        
        // Let background compaction thread start up and run if needed
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto v1 = db.Get("large_key");
        EXPECT_TRUE(v1.has_value());
        EXPECT_EQ(v1.value(), large_val);
    } // DB is closed here, forcing everything to disk
    
    // Test WAL Recovery
    {
        DBImpl db2(db_path);
        auto v2 = db2.Get("small_key");
        EXPECT_TRUE(v2.has_value());
        EXPECT_EQ(v2.value(), "small_val");
    }
}
