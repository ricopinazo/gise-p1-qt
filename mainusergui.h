#ifndef MAINUSERGUI_H
#define MAINUSERGUI_H

#include <QWidget>
#include <QtSerialPort/qserialport.h>
#include <QMessageBox>
#include <QIntValidator>    // Para la validación de la entrada de la tasa de muestreo
#include <qwt_plot_curve.h> // Para usar la gráfica
#include <qwt_plot_grid.h>
#include "qtivarpc.h"

namespace Ui {
class MainUserGUI;
}

class MainUserGUI : public QWidget
{
    Q_OBJECT
    
public: 
    explicit MainUserGUI(QWidget *parent = 0);
    ~MainUserGUI();
    
private slots:
    //Slots asociados por nombre
    void on_runButton_clicked();    
    void on_serialPortComboBox_currentIndexChanged(const QString &arg1);
    void on_pushButton_clicked();    
    void on_colorWheel_colorChanged(const QColor &arg1);
    void on_sondeoButton_clicked();
    void on_rateLineEdit_editingFinished();
    void on_tabWidget_currentChanged(int index);

    //Otros slots
    void cambiaLEDs();
    void samplingConfigChanged();
    void tivaStatusChanged(int status,QString message);
    void pingResponseReceived(void);
    void CommandRejected(int16_t code);
    void buttonsStatusReceived(bool button1, bool button2);
    void buttonsAnswerReceived(bool button1, bool button2);
    void samplesReceived(uint16_t *channel0, uint16_t *channel1, uint16_t *channel2, uint16_t *channel3);
    void channelsActivedChanged();

private:
    // funciones privadas
    void processError(const QString &s);
    void activateRunButton();

private:
    //Componentes privados
    Ui::MainUserGUI *ui;
    int transactionCount;
    QMessageBox ventanaPopUp;
    QTivaRPC tiva; //Objeto para gestionar la ejecucion acciones en el microcontrolador y/o recibir eventos desde él
    QIntValidator *rateLineEditValidator;   // Validador para la entrada de texto de la tasa de muestreo
    // Gráfica
    double xVal[1024];
    double yVal[4][1024];
    QwtPlotCurve *channelCurve[4];
    QwtPlotGrid *grid;
    QCheckBox *channelCheckBox[4];
};

#endif // GUIPANEL_H
