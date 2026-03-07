#pragma once

#include <QtCore/qfile.h>
#include <QtCore/qtimer.h>
#include <QtWidgets/qplaintextedit.h>
#include <QtWidgets/qwidget.h>

class LogWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LogWindow(const QString& logFilePath, QWidget* parent = nullptr);

private slots:
    void pollFile();

private:
    QPlainTextEdit* m_textEdit;
    QTimer          m_pollTimer;
    QFile           m_file;
    qint64          m_filePos = 0;
};
