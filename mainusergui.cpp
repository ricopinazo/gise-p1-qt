#include "mainusergui.h"
#include "ui_mainusergui.h"
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie
#include <QMessageBox>      // Se deben incluir cabeceras a los componentes que se vayan a crear en la clase
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
    ventanaPopUp.setText(tr("Status: RESPUESTA A PING RECIBIDA")); //Este es el texto que muestra la ventana
    ventanaPopUp.setStandardButtons(QMessageBox::Ok);
    ventanaPopUp.setWindowTitle(tr("Evento"));
    ventanaPopUp.setParent(this,Qt::Popup);

    //Configura la validación de la entrada de texto para la tasa de muestreo
    rateLineEditValidator = new QIntValidator();
    rateLineEditValidator->setRange(1,5000);
    ui->rateLineEdit->setValidator(rateLineEditValidator);

    //Configura la gráfica
    for(int k = 0; k < 1024; ++k)
    {
        xVal[k] = (double)k;
        yVal[0][k] = 0.0;
        yVal[1][k] = 0.0;
        yVal[2][k] = 0.0;
        yVal[3][k] = 0.0;
    }

    for(int k = 0; k < 4; ++k)
    {
        channel[k] = new QwtPlotCurve();
        channel[k]->attach(ui->graph);
        channel[k]->setRawSamples(xVal, yVal[k], 1024);
    }

    channel[0]->setPen(Qt::red);
    channel[1]->setPen(Qt::cyan);
    channel[2]->setPen(Qt::yellow);
    channel[3]->setPen(Qt::green);

    grid = new QwtPlotGrid();
    grid->attach(ui->graph);
    ui->graph->setAutoReplot(false);
    ui->graph->setAxisScale(QwtPlot::yLeft, -0.5, 3.5, 0);
    ui->graph->replot();

    //Conexion de signals de los widgets del interfaz con slots propios de este objeto
    connect(ui->rojo,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->verde,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->azul,SIGNAL(toggled(bool)),this,SLOT(cambiaLEDs()));
    connect(ui->activeCheckBox,SIGNAL(toggled(bool)),this,SLOT(samplingConfigChanged()));
    connect(ui->mode8RadioButton,SIGNAL(toggled(bool)),this,SLOT(samplingConfigChanged()));
    connect(ui->rateSlider,SIGNAL(valueChanged(int)),this,SLOT(samplingConfigChanged()));


    //Conectamos Slots del objeto "Tiva" con Slots de nuestra aplicacion (o con widgets)
    connect(&tiva,SIGNAL(statusChanged(int,QString)),this,SLOT(tivaStatusChanged(int,QString)));
    connect(ui->pingButton,SIGNAL(clicked(bool)),&tiva,SLOT(ping()));
    connect(ui->Knob,SIGNAL(valueChanged(double)),&tiva,SLOT(LEDPwmBrightness(double)));
    connect(&tiva,SIGNAL(pingReceivedFromTiva()),this,SLOT(pingResponseReceived()));
    connect(&tiva,SIGNAL(commandRejectedFromTiva(int16_t)),this,SLOT(CommandRejected(int16_t)));
    connect(&tiva,SIGNAL(buttonsStatusReceivedFromTiva(bool,bool)),this,SLOT(buttonsStatusReceived(bool, bool)));
    connect(&tiva,SIGNAL(buttonsAnswerReceivedFromTiva(bool,bool)),this,SLOT(buttonsAnswerReceived(bool, bool)));
    connect(&tiva,SIGNAL(samplesReceivedFromTiva(uint16_t*,uint16_t*,uint16_t*,uint16_t*)),
                       this,SLOT(samplesReceived(uint16_t*,uint16_t*,uint16_t*,uint16_t*)));
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

    tiva.samplingConfig(ui->activeCheckBox->isChecked(), ui->mode12RadioButton->isChecked(), ui->rateSlider->value());
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
    if(ui->activeCheckBox->isChecked())
    {
        for(int k = 0; k < (1024 - 8); ++k)
        {
            yVal[0][k] =  yVal[0][k + 8];
            yVal[1][k] =  yVal[1][k + 8];
            yVal[2][k] =  yVal[2][k + 8];
            yVal[3][k] =  yVal[3][k + 8];
        }

        for(int k = 1024 - 8; k < 1024; ++k)
        {
            yVal[0][k] =  (double)channel0[k]/4095.0*3.3;
            yVal[1][k] =  (double)channel1[k]/4095.0*3.3;
            yVal[2][k] =  (double)channel2[k]/4095.0*3.3;
            yVal[3][k] =  (double)channel3[k]/4095.0*3.3;
        }

        ui->graph->replot();
    }
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
