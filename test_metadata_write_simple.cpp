// Teste simples de escrita de metadados sem UI
#include <gtest/gtest.h>
#include "core/MetadataWriter.h"
#include "core/MetadataReader.h"
#include <QFile>

using namespace PhotoGuru;

TEST(MetadataWriteTest, WriteRatingToHEIC) {
    QString testFile = "/Users/wagnermontes/Documents/GitHub/photoguru/Test_10/test_image.heic";
    
    if (!QFile::exists(testFile)) {
        GTEST_SKIP() << "Test file not found: " << testFile.toStdString();
    }
    
    MetadataWriter& writer = MetadataWriter::instance();
    
    // Teste 1: Escrever Rating
    std::cout << "\n=== Teste 1: Escrevendo Rating ===\n";
    bool success = writer.updateRating(testFile, 4);
    EXPECT_TRUE(success) << "Deveria escrever rating com sucesso";
    
    // Verificar
    auto metadata = MetadataReader::instance().read(testFile);
    if (metadata.has_value()) {
        std::cout << "Rating lido: " << metadata->rating << "\n";
        EXPECT_EQ(metadata->rating, 4) << "Rating deveria ser 4";
    }
    
    // Teste 2: Escrever Title
    std::cout << "\n=== Teste 2: Escrevendo Title ===\n";
    success = writer.updateTitle(testFile, "Test Title from Unit Test");
    EXPECT_TRUE(success) << "Deveria escrever title com sucesso";
    
    // Verificar
    metadata = MetadataReader::instance().read(testFile);
    if (metadata.has_value()) {
        std::cout << "Title lido: " << metadata->llm_title.toStdString() << "\n";
        EXPECT_EQ(metadata->llm_title, "Test Title from Unit Test");
    }
    
    // Teste 3: Escrever Keywords
    std::cout << "\n=== Teste 3: Escrevendo Keywords ===\n";
    QStringList keywords = {"test", "unittest", "photoguru"};
    success = writer.updateKeywords(testFile, keywords);
    EXPECT_TRUE(success) << "Deveria escrever keywords com sucesso";
    
    // Verificar
    metadata = MetadataReader::instance().read(testFile);
    if (metadata.has_value()) {
        std::cout << "Keywords lidos: " << metadata->llm_keywords.size() << " items\n";
        EXPECT_GE(metadata->llm_keywords.size(), 3) << "Deveria ter pelo menos 3 keywords";
    }
}

int main(int argc, char **argv) {
    qputenv("PHOTOGURU_TESTING", "1");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
