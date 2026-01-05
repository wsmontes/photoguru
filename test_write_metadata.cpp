// Teste simples sem UI para debugar escrita de metadados
#include <QCoreApplication>
#include <QImage>
#include <QDebug>
#include <QProcess>
#include <QTemporaryDir>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    // 1. Criar imagem de teste
    QTemporaryDir tempDir;
    QString testFile = tempDir.path() + "/test.jpg";
    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::blue);
    img.save(testFile, "JPEG");
    qDebug() << "Imagem criada:" << testFile;
    
    // 2. Testar ExifTool direto (sem daemon)
    qDebug() << "\n=== TESTE 1: ExifTool direto ===";
    QProcess direct;
    direct.start("exiftool", {
        "-overwrite_original",
        "-XMP:Rating=5",
        testFile
    });
    direct.waitForFinished(5000);
    qDebug() << "Stdout:" << direct.readAllStandardOutput();
    qDebug() << "Stderr:" << direct.readAllStandardError();
    qDebug() << "Exit code:" << direct.exitCode();
    
    // 3. Testar ExifTool em stay-open mode
    qDebug() << "\n=== TESTE 2: ExifTool stay-open mode ===";
    QProcess daemon;
    daemon.start("exiftool", {"-stay_open", "True", "-@", "-"});
    daemon.waitForStarted(3000);
    
    if (daemon.state() == QProcess::Running) {
        qDebug() << "Daemon iniciado!";
        
        // Escrever comando (FORMATO CORRETO segundo documentação)
        QString cmd = QString("-overwrite_original\n-XMP:Title=Test Title\n%1\n-execute\n").arg(testFile);
        qDebug() << "Enviando comando:\n" << cmd;
        daemon.write(cmd.toUtf8());
        daemon.waitForBytesWritten(3000);
        
        // Ler resposta até {ready}
        QByteArray response;
        int timeout = 5000;
        int elapsed = 0;
        while (elapsed < timeout) {
            daemon.waitForReadyRead(100);
            QByteArray data = daemon.readAll();
            response.append(data);
            if (response.contains("{ready}")) {
                break;
            }
            elapsed += 100;
        }
        
        qDebug() << "Resposta daemon:" << response;
        
        // Encerrar daemon
        daemon.write("-stay_open\nFalse\n");
        daemon.waitForFinished(3000);
    } else {
        qDebug() << "ERRO: Daemon não iniciou!";
        qDebug() << "Error:" << daemon.errorString();
    }
    
    // 4. Verificar se metadados foram escritos
    qDebug() << "\n=== VERIFICAÇÃO ===";
    QProcess verify;
    verify.start("exiftool", {"-json", "-a", "-s", testFile});
    verify.waitForFinished(3000);
    qDebug() << "Metadados escritos:\n" << verify.readAllStandardOutput();
    
    return 0;
}
