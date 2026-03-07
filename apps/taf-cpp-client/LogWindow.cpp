#include "LogWindow.h"

#include <QtCore/qfileinfo.h>
#include <QtGui/qtextcursor.h>
#include <QtWidgets/qboxlayout.h>

LogWindow::LogWindow(const QString& logFilePath, QWidget* parent)
    : QWidget(parent, Qt::Window),
      m_file(logFilePath)
{
    setWindowTitle(QFileInfo(logFilePath).fileName());
    resize(900, 500);

    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setMaximumBlockCount(5000);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_textEdit);

    QObject::connect(&m_pollTimer, &QTimer::timeout, this, &LogWindow::pollFile);
    m_pollTimer.start(200);
    pollFile();
}

void LogWindow::pollFile()
{
    if (!m_file.isOpen())
    {
        if (!m_file.exists() || !m_file.open(QIODevice::ReadOnly))
            return;
        m_filePos = 0;
    }

    m_file.seek(m_filePos);
    QByteArray data = m_file.readAll();
    if (data.isEmpty())
        return;

    m_filePos = m_file.pos();
    QString text = QString::fromUtf8(data).replace("\r\n", "\n");
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    m_textEdit->setTextCursor(cursor);
    m_textEdit->ensureCursorVisible();
}
