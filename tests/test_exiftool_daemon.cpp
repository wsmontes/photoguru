#include <gtest/gtest.h>
#include "core/ExifToolDaemon.h"
#include <QFile>
#include <QTemporaryDir>
#include <QImage>
#include <QThread>
#include <QDebug>

using namespace PhotoGuru;

class ExifToolDaemonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        tempDir = new QTemporaryDir();
        ASSERT_TRUE(tempDir->isValid());
        
        // Create a test image
        testImagePath = tempDir->path() + "/daemon_test.jpg";
        QImage img(100, 100, QImage::Format_RGB32);
        img.fill(Qt::red);
        ASSERT_TRUE(img.save(testImagePath, "JPEG", 95));
        
        // Ensure daemon is stopped before each test
        ExifToolDaemon::instance().stop();
    }
    
    void TearDown() override {
        ExifToolDaemon::instance().stop();
        delete tempDir;
    }
    
    QTemporaryDir* tempDir = nullptr;
    QString testImagePath;
};

TEST_F(ExifToolDaemonTest, StartDaemon) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    
    EXPECT_TRUE(daemon.start()) << "Daemon should start successfully";
    EXPECT_TRUE(daemon.isRunning()) << "Daemon should report as running";
}

TEST_F(ExifToolDaemonTest, StopDaemon) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    
    daemon.start();
    ASSERT_TRUE(daemon.isRunning());
    
    daemon.stop();
    EXPECT_FALSE(daemon.isRunning()) << "Daemon should be stopped";
}

TEST_F(ExifToolDaemonTest, RestartDaemon) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    
    // Start, stop, start again
    EXPECT_TRUE(daemon.start());
    daemon.stop();
    EXPECT_TRUE(daemon.start()) << "Should be able to restart daemon";
    EXPECT_TRUE(daemon.isRunning());
}

TEST_F(ExifToolDaemonTest, ExecuteSimpleCommand) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    QStringList args = {"-ver"};
    QString result = daemon.executeCommand(args);
    
    EXPECT_FALSE(result.isEmpty()) << "Should return ExifTool version";
    EXPECT_TRUE(result.contains(".")) << "Version should contain dot (e.g., 13.44)";
    qDebug() << "ExifTool version from daemon:" << result.trimmed();
}

TEST_F(ExifToolDaemonTest, ExecuteReadMetadata) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    QStringList args = {"-json", "-a", "-s", testImagePath};
    QString result = daemon.executeCommand(args);
    
    EXPECT_FALSE(result.isEmpty());
    EXPECT_TRUE(result.contains("[")) << "JSON result should start with [";
    EXPECT_TRUE(result.contains("SourceFile")) << "Should contain SourceFile field";
}

TEST_F(ExifToolDaemonTest, ExecuteWriteMetadata) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    // Write title
    QStringList args = {
        "-overwrite_original",
        "-XMP:Title=Daemon Test Title",
        testImagePath
    };
    QString result = daemon.executeCommand(args);
    
    EXPECT_TRUE(result.contains("1 image files updated") || 
                result.contains("1 image files created"));
    
    // Verify by reading back
    QStringList readArgs = {"-Title", "-s3", testImagePath};
    QString readResult = daemon.executeCommand(readArgs);
    EXPECT_EQ(readResult.trimmed(), "Daemon Test Title");
}

TEST_F(ExifToolDaemonTest, MultipleCommandsSequential) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    // Execute multiple commands in sequence
    for (int i = 0; i < 10; i++) {
        QStringList args = {"-ver"};
        QString result = daemon.executeCommand(args);
        EXPECT_FALSE(result.isEmpty()) << "Command " << i << " should succeed";
    }
}

TEST_F(ExifToolDaemonTest, PerformanceVsIndividualProcess) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    const int iterations = 10;
    
    // Measure daemon performance
    auto startDaemon = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        QStringList args = {"-Title", "-s3", testImagePath};
        daemon.executeCommand(args);
    }
    auto endDaemon = std::chrono::high_resolution_clock::now();
    auto durationDaemon = std::chrono::duration_cast<std::chrono::milliseconds>(
        endDaemon - startDaemon).count();
    
    daemon.stop();
    
    // Measure individual process performance
    auto startIndividual = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        QProcess process;
        process.start("exiftool", {"-Title", "-s3", testImagePath});
        process.waitForFinished();
    }
    auto endIndividual = std::chrono::high_resolution_clock::now();
    auto durationIndividual = std::chrono::duration_cast<std::chrono::milliseconds>(
        endIndividual - startIndividual).count();
    
    qDebug() << "Daemon:" << durationDaemon << "ms";
    qDebug() << "Individual:" << durationIndividual << "ms";
    qDebug() << "Speedup:" << (double)durationIndividual / durationDaemon << "x";
    
    // Daemon should be significantly faster (at least 2x)
    EXPECT_LT(durationDaemon, durationIndividual / 2) 
        << "Daemon should be at least 2x faster";
}

TEST_F(ExifToolDaemonTest, HandleInvalidCommand) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    QStringList args = {"-InvalidOption123", testImagePath};
    QString result = daemon.executeCommand(args);
    
    // Should return error message, not crash
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(ExifToolDaemonTest, HandleNonExistentFile) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    QStringList args = {"-json", "/nonexistent/file.jpg"};
    QString result = daemon.executeCommand(args);
    
    // Should return error, not crash
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(ExifToolDaemonTest, BatchOperations) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    // Create multiple test images
    QStringList imagePaths;
    for (int i = 0; i < 3; i++) {
        QString path = tempDir->path() + QString("/batch_%1.jpg").arg(i);
        QImage img(50, 50, QImage::Format_RGB32);
        img.fill(Qt::blue);
        img.save(path, "JPEG");
        imagePaths << path;
    }
    
    // Prepare batch commands
    QStringList commands;
    for (const QString& path : imagePaths) {
        commands << QString("-Title=%1\n%2").arg("Batch Test", path);
    }
    
    QStringList results = daemon.executeBatch(commands);
    EXPECT_EQ(results.size(), commands.size());
}

TEST_F(ExifToolDaemonTest, ThreadSafety) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    // Create multiple threads that all use the daemon
    const int numThreads = 5;
    QVector<QThread*> threads;
    
    for (int i = 0; i < numThreads; i++) {
        QThread* thread = QThread::create([this, &daemon, i]() {
            for (int j = 0; j < 5; j++) {
                QStringList args = {"-ver"};
                QString result = daemon.executeCommand(args);
                EXPECT_FALSE(result.isEmpty()) 
                    << "Thread " << i << " command " << j << " failed";
            }
        });
        threads.append(thread);
        thread->start();
    }
    
    // Wait for all threads
    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }
}

TEST_F(ExifToolDaemonTest, LargeOutput) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    ASSERT_TRUE(daemon.start());
    
    // Request verbose JSON output (large)
    QStringList args = {"-json", "-a", "-G1", testImagePath};
    QString result = daemon.executeCommand(args);
    
    EXPECT_FALSE(result.isEmpty());
    EXPECT_GT(result.length(), 100) << "Should return substantial JSON data";
}

TEST_F(ExifToolDaemonTest, SingletonBehavior) {
    ExifToolDaemon& daemon1 = ExifToolDaemon::instance();
    ExifToolDaemon& daemon2 = ExifToolDaemon::instance();
    
    // Should be the same instance
    EXPECT_EQ(&daemon1, &daemon2) << "Should return same singleton instance";
}

TEST_F(ExifToolDaemonTest, StartWhenAlreadyRunning) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    
    ASSERT_TRUE(daemon.start());
    EXPECT_TRUE(daemon.isRunning());
    
    // Try to start again - should be idempotent
    EXPECT_TRUE(daemon.start()) << "Starting already-running daemon should succeed";
    EXPECT_TRUE(daemon.isRunning());
}

TEST_F(ExifToolDaemonTest, ExecuteWithoutStart) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    
    // Try to execute without starting
    QStringList args = {"-ver"};
    QString result = daemon.executeCommand(args);
    
    // Should either auto-start or return empty/error gracefully
    // Implementation dependent, but should not crash
    SUCCEED() << "Did not crash when executing without start";
}

TEST_F(ExifToolDaemonTest, RapidStartStop) {
    ExifToolDaemon& daemon = ExifToolDaemon::instance();
    
    // Rapidly start/stop
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(daemon.start());
        daemon.stop();
    }
    
    SUCCEED() << "Handled rapid start/stop cycles";
}
