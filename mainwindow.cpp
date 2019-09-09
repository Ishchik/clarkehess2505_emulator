#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTextCursor>
#include <QDebug>

const auto infos = QSerialPortInfo::availablePorts();
bool autoreceive = false, autosend = false, avail_data = false;
int timer_interval = 100;
QString line;
std::string start_line = "GAINPH?\r\n";
std::string abort_line = "ABORT\r\n";
qint32 send_state = 1, line_index = 2, step = 1;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    receive_timer->setInterval(timer_interval);
    date_timer->setInterval(timer_interval);

    connect(ui->check_button, SIGNAL(clicked()), this, SLOT(checkPorts()));
    connect(ui->open_button, SIGNAL(clicked()), this, SLOT(connectPort()));
    connect(ui->close_button, SIGNAL(clicked()), this, SLOT(discPort()));
    connect(ui->start_button, SIGNAL(clicked()), this, SLOT(autoSendData()));
    connect(ui->abort_button, SIGNAL(clicked()), this, SLOT(abort()));
    connect(receive_timer, SIGNAL(timeout()), this, SLOT(autoReceive()));
    connect(date_timer, SIGNAL(timeout()), this, SLOT(updateDateTime()));
    connect(delay_timer, SIGNAL(timeout()), this, SLOT(sendData()));
    connect(ui->send_button, SIGNAL(clicked()), this, SLOT(sendFieldData()));
    connect(ui->save_log_button, SIGNAL(clicked()), this, SLOT(saveFile()));
    connect(ui->open_table_button, SIGNAL(clicked()), this, SLOT(loadFile()));


    date_timer->start();
    receive_timer->start();

    checkPorts();


}

MainWindow::~MainWindow()
{
    if(serial->isOpen()){
        serial->close();
    }

    date_timer->stop();

    if(receive_timer->isActive()){
        receive_timer->stop();
    }

    if(delay_timer->isActive()){
        delay_timer->stop();
    }

    delete ui;
}


void MainWindow::checkPorts()
{
    ui->Cport_box->clear();
    for(const QSerialPortInfo &info: infos){
        if(!info.isBusy()){
            ui->Cport_box->addItem(info.portName());
        }
    }
}

void MainWindow::connectPort()
{
    openPort();
}

void MainWindow::discPort()
{
    serial->close();

    ui->open_button->setEnabled(true);
    ui->close_button->setEnabled(false);
    ui->Cport_box->setEnabled(true);
    ui->lineEdit->setEnabled(false);
    ui->send_button->setEnabled(false);
    ui->delay_field->setEnabled(false);
    ui->abort_button->setEnabled(false);
    ui->start_button->setEnabled(false);
    ui->field_send->setChecked(true);
    ui->field_send->setEnabled(false);
    ui->table_send->setChecked(false);
    ui->table_send->setEnabled(false);
    ui->open_table_button->setEnabled(false);
    ui->textEdit_3->clear();
    ui->file_name_edit->clear();
    ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString("hh:mm:ss.zzz") + " " + "Port " + serial->portName() + " closed\n");

    send_state = 1;

    checkPorts();
}

void MainWindow::autoReceive()
{
    if(serial->canReadLine()){
        receive_data = serial->readLine();
        ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString() + " " + receive_data + " received\n");
        qint32 a = receive_data.toStdString() == abort_line;
        qint32 s = receive_data.toStdString() == start_line;
        if(s){
            avail_data = true;
        }else{
            avail_data = false;
        }
        if(a){
            avail_data = false;
            abort();
            step = 1;
        }
    }
}

void MainWindow::sendFieldData()
{
    if(ui->lineEdit->text() == ""){
        QMessageBox::warning(this, "Warning", "Fill the field", QMessageBox::StandardButton::Ok);
        ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString() + " " + "Not enough data to send. Current state: 1\n");
    }
    else{
        send_data_field = ui->lineEdit->text().toUtf8();     //convert string to byteArray
        serial->write(send_data_field + 13 + 10);        //send data
        ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString() + " " + send_data_field + " " + "sent\n");

        if(!serial->waitForBytesWritten()){
            QMessageBox::critical(this, "Error while sending data", serial->errorString(), QMessageBox::StandardButton::Cancel);
        }
    }
}

void MainWindow::autoSendData()
{
    if(ui->delay_field->text().toInt()){
        ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString() + " " + "Start\n");
        autosend = true;
        ui->start_button->setEnabled(false);
        ui->delay_field->setEnabled(false);
        ui->abort_button->setEnabled(true);
        delay_timer->setInterval(ui->delay_field->text().toInt());
        delay_timer->start();
    }
}

void MainWindow::abort()
{
    if(autosend){
        ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString() + " " + "Abort\n");
        autosend = false;
        ui->start_button->setEnabled(true);
        ui->delay_field->setEnabled(true);
        ui->abort_button->setEnabled(false);

        if(delay_timer->isActive()){
            delay_timer->stop();
        }
    }
    serial->clear();
}

void MainWindow::updateDateTime()
{
    cdt.setDate(QDate::currentDate());
    cdt.setTime(QTime::currentTime());
}

void MainWindow::saveFile()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Save log", "", "Text files (*.txt)");
    if(!file_name.isEmpty()){
        QFile file(file_name);
        if(file.open(QFile::WriteOnly)){
            file.write(ui->textEdit->toPlainText().toUtf8());
        }
        else{
            QMessageBox::critical(this, "Error", tr("Cannot write file %1.\nError: %2").arg(file_name).arg(file.errorString()));
            return;
        }
        file.close();
    }
}

void MainWindow::loadFile()
{
    ui->textEdit_3->clear();
    QString file_name = QFileDialog::getOpenFileName(this, "Load Table", "", "Text files (*.txt)");
    if(!file_name.isEmpty()){
        QFile file(file_name);
        QString line;
        ui->textEdit_3->clear();
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
            QTextStream stream(&file);
            ui->file_name_edit->setText(file_name);
            while(!stream.atEnd()){
                line = stream.readLine();
                ui->textEdit_3->insertPlainText(line + "\n");
            }
        }
        else{
            QMessageBox::critical(this, "Error", tr("Cannot load file %1.\nError: %2").arg(file_name).arg(file.errorString()));
            return;
        }
        file.close();

        QTextCursor text_cursor = ui->textEdit_3->textCursor();

        text_cursor.setPosition(1);
        text_cursor.movePosition(QTextCursor::Down, QTextCursor::MoveMode::MoveAnchor, 2 + step);
        text_cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveMode::MoveAnchor);

        step = 1;
    }
}

void MainWindow::openPort()
{
    serial->setPortName(ui->Cport_box->currentText());
    serial->setBaudRate(QSerialPort::Baud19200, QSerialPort::AllDirections);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if(serial->open(QIODevice::ReadWrite)){
        ui->open_button->setEnabled(false);
        ui->close_button->setEnabled(true);
        ui->Cport_box->setEnabled(false);
        ui->field_send->setChecked(true);
        if(ui->field_send->isChecked()){
            ui->lineEdit->setEnabled(true);
            ui->lineEdit_2->setEnabled(false);
            ui->send_button->setEnabled(true);
            ui->abort_button->setEnabled(false);
            ui->start_button->setEnabled(false);
            send_state = 1;
            step = 1;
        }
        ui->field_send->setEnabled(true);
        ui->table_send->setEnabled(true);
        ui->auto_send_radio->setEnabled(true);
        ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString("hh:mm:ss.zzz") + " " + "Port " + serial->portName() + " opened\n");
    }
    else{
        QMessageBox::critical(this, "Error while openning port", serial->errorString(), QMessageBox::StandardButton::Cancel);
    }
}

void MainWindow::on_field_send_clicked()
{
    ui->lineEdit->setEnabled(true);
    ui->lineEdit_2->setEnabled(false);
    ui->send_button->setEnabled(true);
    ui->open_table_button->setEnabled(false);
    ui->delay_field->setEnabled(false);
    ui->start_button->setEnabled(false);
    ui->abort_button->setEnabled(false);
    send_state = 1;
}

void MainWindow::on_table_send_clicked()
{
    if(ui->textEdit_3->toPlainText() != ""){
        QTextCursor text_cursor = ui->textEdit_3->textCursor();

        text_cursor.setPosition(1);
        text_cursor.movePosition(QTextCursor::Down, QTextCursor::MoveMode::MoveAnchor, 2 + step);
        text_cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveMode::MoveAnchor);
    }
    ui->lineEdit->setEnabled(false);
    ui->lineEdit_2->setEnabled(false);
    ui->send_button->setEnabled(false);
    ui->open_table_button->setEnabled(true);
    ui->delay_field->setEnabled(true);
    ui->start_button->setEnabled(true);
    ui->abort_button->setEnabled(true);
    send_state = 2;
}


void MainWindow::on_auto_send_radio_clicked()
{
    ui->lineEdit->setEnabled(false);
    ui->lineEdit_2->setEnabled(true);
    ui->send_button->setEnabled(false);
    ui->open_table_button->setEnabled(false);
    ui->delay_field->setEnabled(true);
    ui->start_button->setEnabled(true);
    ui->abort_button->setEnabled(true);
    send_state = 3;
}

void MainWindow::sendData()
{
    if(avail_data){
        switch (send_state) {
        case 2:
        {
            if(ui->textEdit_3->toPlainText() == ""){
                ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString() + " " + "Not enough data to send. Current state: 2\n");
            }
            else{
                QTextCursor text_cursor = ui->textEdit_3->textCursor();

                text_cursor.setPosition(1);
                text_cursor.movePosition(QTextCursor::Down, QTextCursor::MoveMode::MoveAnchor, 2 + step);
                text_cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveMode::MoveAnchor);
                text_cursor.selectionStart();
                text_cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveMode::KeepAnchor);
                text_cursor.selectionEnd();

                ui->textEdit_3->setTextCursor(text_cursor);

                line = text_cursor.selectedText();
                send_data = line.toUtf8();
                serial->write(send_data + 13 + 10);
                ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString() + " " + send_data + " " + "sent\n");
                step++;

                if(text_cursor.atEnd()){
                    abort();
                    step = 1;
                }
                }
            }

            break;
        case 3:
        {
            if(ui->lineEdit_2->text() == ""){
                ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString() + " " + "Not enough data to send. Current state: 3\n");
            }
            else{
                send_data = ui->lineEdit_2->text().toUtf8();
                serial->write(send_data + 13 + 10);
                ui->textEdit->insertPlainText(cdt.date().toString(Qt::SystemLocaleShortDate) + " " + cdt.time().toString() + " " + send_data + " " + "sent\n");
                break;
            }
        }
        }
    }
}
