#include "mainusergui.h"
#include "ui_mainusergui.h"
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie
#include <QMessageBox>      // Se deben incluir cabeceras a los componentes que se vayan a crear en la clase
#include <QTimer>           // Empleado para apagar la alarma pasao un tiempo
// y que no estén incluidos en el interfaz gráfico. En este caso, la ventana de PopUp <QMessageBox>
// que se muestra al recibir un PING de respuesta

#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos

MainUserGUI::MainUserGUI(QWidget *parent) :  // Constructor de la clase
    QWidget(parent),
    ui(new Ui::MainUserGUI)               // Indica que guipanel.ui es el interfaz grafico de la clase
  , transactionCount(0)
{
    ui->setupUi(this);                // Conecta la clase con su interfaz gráfico.
    setWindowTitle(tr("Interfaz de Control TIVA")); // Título de la ventana

    // Inicializa la lista de puertos serie
    ui->serialPortComboBox->clear(); // Vacía de componentes la comboBox
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        // La descripción realiza un FILTRADO que  nos permite que SOLO aparezcan los interfaces tipo USB serial con el identificador de fabricante
        // y producto de la TIVA
        // NOTA!!: SI QUIERES REUTILIZAR ESTE CODIGO PARA OTRO CONVERSOR USB-Serie, cambia los valores VENDOR y PRODUCT por los correspondientes
        // o cambia la condicion por "if (true) para listar todos los puertos serie"
        if ((info.vendorIdentifier()==0x1CBE) && (info.productIdentifier()==0x0002))
        {
            ui->serialPortComboBox->addItem(info.portName());
        }
    }

    //Inicializa algunos controles
    ui->serialPortComboBox->setFocus();   // Componente del GUI seleccionado de inicio
    ui->pingButton->setEnabled(false);    // Se deshabilita el botón de ping del interfaz gráfico, hasta que

    //Inicializa la ventana de PopUp que se muestra cuando llega la respuesta al PING
    ventanaPopUp.setIcon(QMessageBox::Information);
    ventanaPopUp.setStandardButtons(QMessageBox::Ok);
    ventanaPopUp.setWindowTitle(tr("Evento"));
    ventanaPopUp.setParent(this,Qt::Popup);

    //Configura la validación de la entrada de texto para la tasa de muestreo
    rateLineEditValidator = new QIntValidator();
    rateLineEditValidator->setRange(1,5000);
    ui->rateLineEdit->setValidator(rateLineEditValidator);

    //Configura la gráfica
    for(int k = 0; k < GRAPH_WIDTH; ++k)
    {
        xVal[k] = (double)k;
        adcVal[0][k] = 0.0;
        adcVal[1][k] = 0.0;
        adcVal[2][k] = 0.0;
        adcVal[3][k] = 0.0;
        sensorVal[0][k] = 0.0;
        sensorVal[1][k] = 0.0;
        sensorVal[2][k] = 0.0;
        sensorVal[3][k] = 0.0;
    }

    for(int k = 0; k < 4; ++k)
    {
        channelCurve[k] = new QwtPlotCurve();
        channelCurve[k]->attach(ui->graph);
    }

    channelCurve[0]->setPen(Qt::red);
    channelCurve[1]->setPen(Qt::cyan);
    channelCurve[2]->setPen(Qt::yellow);
    channelCurve[3]->setPen(Qt::green);

    grid = new QwtPlotGrid();
    grid->attach(ui->graph);

    ui->graph->setAutoReplot(false);
    ui->graph->setAxisScale(QwtPlot::yLeft, -0.5, 3.5, 0);
    ui->graph->setAxisScale(QwtPlot::xBottom, 0.0, 1024.0, 0);
    ui->graph->replot();

    channelCheckBox[0] = ui->channel0CheckBox;
    channelCheckBox[1] = ui->channel1CheckBox;
    channelCheckBox[2] = ui->channel2CheckBox;
    channelCheckBox[3] = ui->channel3CheckBox;

    // Inicializa el timer de la alarma
    alarm_timer = new QTimer();
    alarm_timer->setSingleShot(true);

    //Conexion de signals de los widgets del interfaz con slots propios de este objeto
    connect(ui->rojo,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->verde,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->azul,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->mode8RadioButton,SIGNAL(toggled(bool)),this,SLOT(samplingConfigChanged()));
    connect(ui->rateSlider,SIGNAL(valueChanged(int)),this,SLOT(samplingConfigChanged()));
    for(int k = 0; k < 4; ++k)
        connect(channelCheckBox[k],SIGNAL(toggled(bool)),this,SLOT(channelsActivedChanged()));
    connect(alarm_timer,SIGNAL(timeout()),this,SLOT(stopAlarmLed()));

    //Conectamos Slots del objeto "Tiva" con Slots de nuestra aplicacion (o con widgets)
    connect(&tiva,SIGNAL(statusChanged(int,QString)),this,SLOT(tivaStatusChanged(int,QString)));
    connect(ui->pingButton,SIGNAL(clicked(bool)),&tiva,SLOT(ping()));
    connect(ui->Knob,SIGNAL(valueChanged(double)),&tiva,SLOT(LEDPwmBrightness(double)));
    connect(ui->threshold_spin_box,SIGNAL(valueChanged(int)),&tiva,SLOT(configThreshold(int)));
    connect(&tiva,SIGNAL(pingReceivedFromTiva()),this,SLOT(pingResponseReceived()));
    connect(&tiva,SIGNAL(commandRejectedFromTiva(int16_t)),this,SLOT(CommandRejected(int16_t)));
    connect(&tiva,SIGNAL(buttonsStatusReceivedFromTiva(bool,bool)),this,SLOT(buttonsStatusReceived(bool, bool)));
    connect(&tiva,SIGNAL(buttonsAnswerReceivedFromTiva(bool,bool)),this,SLOT(buttonsAnswerReceived(bool, bool)));
    connect(&tiva,SIGNAL(samplesReceivedFromTiva(uint16_t*,uint16_t*,uint16_t*,uint16_t*)),
            this,SLOT(samplesReceived(uint16_t*,uint16_t*,uint16_t*,uint16_t*)));
    connect(&tiva,SIGNAL(colorReceivedFromTiva(uint16_t,uint16_t,uint16_t,uint16_t)),
            this,SLOT(colorReceived(uint16_t,uint16_t,uint16_t,uint16_t)));
    connect(&tiva,SIGNAL(gestureReceivedFromTiva(uint8_t)),this,SLOT(gestureReceived(uint8_t)));
    connect(&tiva,SIGNAL(proximityAlarmReceivedFromTiva()),this,SLOT(proximityAlarmReceived()));
    connect(&tiva,SIGNAL(fifoReceivedFromTiva(uint8_t*,uint8_t)),this,SLOT(fifoReceived(uint8_t*,uint8_t)));
}

MainUserGUI::~MainUserGUI() // Destructor de la clase
{
    delete ui;   // Borra el interfaz gráfico asociado a la clase
}

void MainUserGUI::on_serialPortComboBox_currentIndexChanged(const QString &arg1)
{
    activateRunButton();
}

// Funcion auxiliar de procesamiento de errores de comunicación
void MainUserGUI::processError(const QString &s)
{
    activateRunButton(); // Activa el botón RUN
    // Muestra en la etiqueta de estado la razón del error (notese la forma de pasar argumentos a la cadena de texto)
    ui->statusLabel->setText(tr("Status: Not running, %1.").arg(s));
}

// Funcion de habilitacion del boton de inicio/conexion
void MainUserGUI::activateRunButton()
{
    ui->runButton->setEnabled(true);
}

//Este Slot lo hemos conectado con la señal statusChange de la TIVA, que se activa cuando
//El puerto se conecta o se desconecta y cuando se producen determinados errores.
//Esta función lo que hace es procesar dichos eventos
void MainUserGUI::tivaStatusChanged(int status,QString message)
{
    switch (status)
    {
        case QTivaRPC::TivaConnected:

            //Caso conectado..
            // Deshabilito el boton de conectar
            ui->runButton->setEnabled(false);

            // Se indica que se ha realizado la conexión en la etiqueta 'statusLabel'
            ui->statusLabel->setText(tr("Ejecucion, conectado al puerto %1.")
                             .arg(ui->serialPortComboBox->currentText()));

            //    // Y se habilitan los controles deshabilitados
            ui->pingButton->setEnabled(true);

        break;

        case QTivaRPC::TivaDisconnected:
            //Caso desconectado..
            // Rehabilito el boton de conectar
            ui->runButton->setEnabled(false);
        break;
        case QTivaRPC::UnexpectedPacketError:
        case QTivaRPC::FragmentedPacketError:
        case QTivaRPC::CRCorStuffError:
            //Errores detectados en la recepcion de paquetes
            ui->statusLabel->setText(message);
        break;
        default:
            //Otros errores (por ejemplo, abriendo el puerto)
            processError(tiva.getLastErrorMessage());
    }
}


// SLOT asociada a pulsación del botón RUN
void MainUserGUI::on_runButton_clicked()
{
    //Intenta arrancar la comunicacion con la TIVA
    tiva.startRPCClient( ui->serialPortComboBox->currentText());
}

void MainUserGUI::cambiaLEDs(void)
{
    //Invoca al metodo LEDGPio para enviar la orden a la TIVA
    tiva.LEDGpio(ui->rojo->isChecked(),ui->verde->isChecked(),ui->azul->isChecked());
}

void MainUserGUI::samplingConfigChanged()
{
    // Actualiza el estado del cuadro de texto por si se hubiese movido el slider
    QString str;
    str.setNum(ui->rateSlider->value());
    ui->rateLineEdit->setText(str);

    tiva.samplingConfig(ui->adcCheckBox->isChecked(), ui->mode12RadioButton->isChecked(), ui->rateSlider->value());
}

//Slots Asociado al boton que limpia los mensajes del interfaz
void MainUserGUI::on_pushButton_clicked()
{
    ui->statusLabel->setText(tr(""));
}

//**** Slots asociados a la recepción de mensajes desde la TIVA ********/
//Están conectados a señales generadas por el objeto TIVA de clase QTivaRPC (se conectan en el constructor de la ventana, más arriba en este fichero))
//Se pueden añadir los que se quieran para completar la aplicación

//Este se ejecuta cuando se recibe una respuesta de PING
void MainUserGUI::pingResponseReceived()
{
    // Muestra una ventana popUP para el caso de comando PING; no te deja definirla en un "caso"
    ventanaPopUp.setText(tr("Status: RESPUESTA A PING RECIBIDA")); //Este es el texto que muestra la ventana
    ventanaPopUp.setStyleSheet("background-color: lightgrey");
    ventanaPopUp.setModal(true); //CAMBIO: Se sustituye la llamada a exec(...) por estas dos.
    ventanaPopUp.show();
}

//Este se ejecuta cuando se recibe un mensaje de comando rechazado
void MainUserGUI::CommandRejected(int16_t code)
{
    ui->statusLabel->setText(tr("Status: Comando rechazado,%1").arg(code));
}

void MainUserGUI::buttonsStatusReceived(bool button1, bool button2)
{
    // Actualiza el estado de los leds en consecuencia
    if (ui->modoCheck->isChecked())
    {
        ui->led1->setChecked(button1);
        ui->led2->setChecked(button2);
    }
}

void MainUserGUI::buttonsAnswerReceived(bool button1, bool button2)
{
    // Actualiza el estado de los leds en consecuencia
    ui->led1->setChecked(button1);
    ui->led2->setChecked(button2);
}

void MainUserGUI::samplesReceived(uint16_t *channel0, uint16_t *channel1, uint16_t *channel2, uint16_t *channel3)
{
    if(ui->adcCheckBox->isChecked())
    {
        for(int k = 0; k < (GRAPH_WIDTH - 8); ++k)
        {
            adcVal[0][k] =  adcVal[0][k + 8];
            adcVal[1][k] =  adcVal[1][k + 8];
            adcVal[2][k] =  adcVal[2][k + 8];
            adcVal[3][k] =  adcVal[3][k + 8];
        }

        for(int k = 0; k < 8; ++k)
        {
            adcVal[0][k + GRAPH_WIDTH - 8] =  (double)channel0[k]/4095.0*3.3;
            adcVal[1][k + GRAPH_WIDTH - 8] =  (double)channel1[k]/4095.0*3.3;
            adcVal[2][k + GRAPH_WIDTH - 8] =  (double)channel2[k]/4095.0*3.3;
            adcVal[3][k + GRAPH_WIDTH - 8] =  (double)channel3[k]/4095.0*3.3;
        }

        ui->graph->replot();
    }
}

void MainUserGUI::channelsActivedChanged()
{
    for(int k = 0; k < 4; ++k)
    {
        if(channelCheckBox[k]->isChecked())
            channelCurve[k]->attach(ui->graph);
        else
            channelCurve[k]->detach();
    }
    ui->graph->replot();
}

void MainUserGUI::colorReceived(uint16_t red, uint16_t green, uint16_t blue, uint16_t intensity)
{
    intensity = (int)(((float)intensity)/2.55);
    if(intensity > 100)
        intensity = 100;
    if (red > 255)
        red = 255;
    if (green > 255)
        green = 255;
    if (blue > 255)
        blue = 255;


    ui->color_preview->setColor(QColor(red,green,blue));
    ui->bright_label->setText(QString::number(intensity));
}

void MainUserGUI::gestureReceived(uint8_t gesture)
{
    QString gesture_text;

    switch ((Gesture)gesture)
    {
        case SF_APDS9960_DIR_ALL:
            gesture_text = "Todas";
            break;
        case SF_APDS9960_DIR_DOWN:
            gesture_text = "Abajo";
            break;
        case SF_APDS9960_DIR_FAR:
            gesture_text = "Atrás";
            break;
        case SF_APDS9960_DIR_LEFT:
            gesture_text = "Izquierda";
            break;
        case SF_APDS9960_DIR_NEAR:
            gesture_text = "Adelante";
            break;
        case SF_APDS9960_DIR_NONE:
            gesture_text = "Ninguna";
            break;
        case SF_APDS9960_DIR_RIGHT:
            gesture_text = "Derecha";
            break;
        case SF_APDS9960_DIR_UP:
            gesture_text = "Arriba";
            break;
        default:
            gesture_text = "Ninguna";
            break;
    }

    ui->gesture_label->setText(gesture_text);
}

void MainUserGUI::proximityAlarmReceived()
{
    ui->alarm_led->setChecked(true);
    alarm_timer->start(1000);  // Pasados 1000 milisegundos, se apaga el led
}

void MainUserGUI::stopAlarmLed()
{
    ui->alarm_led->setChecked(false);
}

void MainUserGUI::on_colorWheel_colorChanged(const QColor &qcolor)
{
    tiva.LEDPwmColor(qcolor.red(), qcolor.green(), qcolor.blue());
}

void MainUserGUI::on_sondeoButton_clicked()
{
    if(!ui->modoCheck->isChecked()) //Si el modo automático no está activado...
    {
        tiva.buttonsRequest();  // Realiza una petición de sondeo
    }
}

void MainUserGUI::on_rateLineEdit_editingFinished()
{
    ui->rateSlider->setValue(ui->rateLineEdit->text().toInt());
}

void MainUserGUI::on_tabWidget_currentChanged(int index)
{
    if(index == 0)
    {
        // Cuando se pasa a la pestaña de Colores, se detiene el muestreado en la tiva
        tiva.samplingConfig(false, ui->mode12RadioButton->isChecked(), ui->rateSlider->value());
        tiva.configSensorMode(false);
    }
    else if (index == 1)
    {
        QString str;
        str.setNum(ui->rateSlider->value());
        ui->rateLineEdit->setText(str);
        tiva.samplingConfig(ui->adcCheckBox->isChecked(), ui->mode12RadioButton->isChecked(), ui->rateSlider->value());
        tiva.configSensorMode(ui->sensorCheckBox->isChecked());
    }
}

void MainUserGUI::on_color_button_clicked()
{
    tiva.colorRequest(); // Realiza el envío del comando de petición de color
}

void MainUserGUI::on_adcCheckBox_toggled(bool checked)
{
    samplingConfigChanged();

    if(checked)
    {
        ui->sensorCheckBox->setChecked(false);
        ui->graph->setAxisScale(QwtPlot::yLeft, 0.0, 3.5, 0);
        ui->graph->replot();
        for(int k = 0; k < 4; ++k)
            channelCurve[k]->setRawSamples(xVal, adcVal[k], GRAPH_WIDTH);

    }

    ui->label_3->setEnabled(checked);
    ui->label_4->setEnabled(checked);
    ui->label_5->setEnabled(checked);
    ui->label_6->setEnabled(checked);
    ui->rateLineEdit->setEnabled(checked);
    ui->rateSlider->setEnabled(checked);
    ui->mode8RadioButton->setEnabled(checked);
    ui->mode12RadioButton->setEnabled(checked);
}

void MainUserGUI::on_sensorCheckBox_toggled(bool checked)
{
    tiva.configSensorMode(checked);
    if(checked)
    {
        ui->adcCheckBox->setChecked(false);
        ui->graph->setAxisScale(QwtPlot::yLeft, 0.0, 255.0, 0);
        ui->graph->replot();
        for(int k = 0; k < 4; ++k)
            channelCurve[k]->setRawSamples(xVal, sensorVal[k], GRAPH_WIDTH);
    }
}

void MainUserGUI::fifoReceived(uint8_t *fifo, uint8_t size)
{
    int samples = size/4;

    if(ui->sensorCheckBox->isChecked())
    {
        for(int k = 0; k < (GRAPH_WIDTH - samples); ++k)
        {
            sensorVal[0][k] =  sensorVal[0][k + samples];
            sensorVal[1][k] =  sensorVal[1][k + samples];
            sensorVal[2][k] =  sensorVal[2][k + samples];
            sensorVal[3][k] =  sensorVal[3][k + samples];
        }

        for(int k = 0; k < samples; k+=4)
        {
            sensorVal[0][k + GRAPH_WIDTH - samples] =  (double)fifo[k];
            sensorVal[1][k + GRAPH_WIDTH - samples] =  (double)fifo[k+1];
            sensorVal[2][k + GRAPH_WIDTH - samples] =  (double)fifo[k+2];
            sensorVal[3][k + GRAPH_WIDTH - samples] =  (double)fifo[k+3];
        }
        ui->graph->replot();
    }
}
