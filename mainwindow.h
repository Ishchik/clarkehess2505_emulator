#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <qtextcursor.h>
#include <QTextStream>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void openPort();

    void checkPorts();

    void connectPort();

    void discPort();

    void autoReceive();

    void sendFieldData();

    void autoSendData();

    void abort();

    void updateDateTime();

    void saveFile();

    void loadFile();

    void on_field_send_clicked();

    void on_table_send_clicked();

    void on_auto_send_radio_clicked();

    void sendData();

private:
    Ui::MainWindow *ui;

    QDateTime cdt = QDateTime::currentDateTime();

    QSerialPort *serial = new QSerialPort();

    QTimer *receive_timer = new QTimer(this);

    QTimer *date_timer = new QTimer(this);

    QTimer *delay_timer = new QTimer(this);

    QByteArray receive_data, send_data_field, send_data;

    QTextCursor text_cursor;
};

#endif // MAINWINDOW_H
