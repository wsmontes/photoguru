#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include "../src/ml/PythonAnalysisWorker.h"

using namespace PhotoGuru;

class PythonAnalysisWorkerTest : public ::testing::Test {
protected:
    void SetUp() override {
        worker = new PythonAnalysisWorker();
    }
    
    void TearDown() override {
        delete worker;
    }
    
    PythonAnalysisWorker* worker = nullptr;
};

// Test worker creation and signals
TEST_F(PythonAnalysisWorkerTest, WorkerInitialization) {
    ASSERT_NE(worker, nullptr);
}

// Test signal emission structure
TEST_F(PythonAnalysisWorkerTest, SignalStructure) {
    QSignalSpy progressSpy(worker, &PythonAnalysisWorker::progress);
    QSignalSpy logSpy(worker, &PythonAnalysisWorker::logMessage);
    QSignalSpy errorSpy(worker, &PythonAnalysisWorker::error);
    QSignalSpy finishedSpy(worker, &PythonAnalysisWorker::finished);
    
    EXPECT_TRUE(progressSpy.isValid());
    EXPECT_TRUE(logSpy.isValid());
    EXPECT_TRUE(errorSpy.isValid());
    EXPECT_TRUE(finishedSpy.isValid());
}

// Test cancel functionality
TEST_F(PythonAnalysisWorkerTest, CancelOperation) {
    EXPECT_NO_THROW(worker->cancel());
}

// Test analyzeImage method exists
TEST_F(PythonAnalysisWorkerTest, AnalyzeImageMethod) {
    // Just verify method can be called without crashing
    // Actual Python integration would require mock Python environment
    EXPECT_NO_THROW(worker->analyzeImage("/nonexistent/test.jpg", false));
}

// Test analyzeDirectory method exists
TEST_F(PythonAnalysisWorkerTest, AnalyzeDirectoryMethod) {
    EXPECT_NO_THROW(worker->analyzeDirectory("/nonexistent/dir", true));
}

// Test findDuplicates method exists
TEST_F(PythonAnalysisWorkerTest, FindDuplicatesMethod) {
    EXPECT_NO_THROW(worker->findDuplicates("/nonexistent/dir", 10));
}

// Test detectBursts method exists
TEST_F(PythonAnalysisWorkerTest, DetectBurstsMethod) {
    EXPECT_NO_THROW(worker->detectBursts("/nonexistent/dir", 5, 2));
}

// Test generateQualityReport method exists
TEST_F(PythonAnalysisWorkerTest, GenerateQualityReportMethod) {
    EXPECT_NO_THROW(worker->generateQualityReport("/nonexistent/dir", "overall"));
}

// Test parameter variations
TEST_F(PythonAnalysisWorkerTest, ParameterVariations) {
    EXPECT_NO_THROW(worker->analyzeImage("test.jpg", true));
    EXPECT_NO_THROW(worker->analyzeDirectory(".", false));
    EXPECT_NO_THROW(worker->findDuplicates(".", 5));
    EXPECT_NO_THROW(worker->detectBursts(".", 10, 3));
    EXPECT_NO_THROW(worker->generateQualityReport(".", "sharpness"));
}
