#ifndef QTIVARPC_H
#define QTIVARPC_H

#include <QObject>
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie


#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos




class QTivaRPC : public QObject
{

    Q_OBJECT
public:
    explicit QTivaRPC(QObject *parent = 0);

    //Define una serie de etiqueta para los errores y estados notificados por la señal statusChanged(...)
    enum TivaStatus {TivaConnected,
                     TivaDisconnected,
                     OpenPortError,
                     BaudRateError,
                     DataBitError,
                     ParityError,
                     StopError,
                     FlowControlError,
                     UnexpectedPacketError,
                     FragmentedPacketError,
                     CRCorStuffError
                    };
    Q_ENUM(TivaStatus)

    //Metodo publico
    QString getLastErrorMessage();

signals:
    void statusChanged(int status, QString message); //Esta señal se genera al realizar la conexión/desconexion o cuando se produce un error de comunicacion
    void pingReceivedFromTiva(void); //Esta señal se genera al recibir una respuesta de PING de la TIVA
    void commandRejectedFromTiva(int16_t code); //Esta señal se genera al recibir una respuesta de Comando Rechazado desde la TIVA
    void buttonsStatusReceivedFromTiva(bool button1, bool button2); //Esta señal se genera cuando se recibe el estado de los botones
    void buttonsAnswerReceivedFromTiva(bool button1, bool button2); //Esta señal se genera cuando se recibe el la respuesta de los botones
    void samplesReceivedFromTiva(uint16_t *channel0, uint16_t *channel1, uint16_t *channel2, uint16_t *channel3);
    void colorReceivedFromTiva(uint16_t red, uint16_t green, uint16_t blue, uint16_t intensity);
    void gestureReceivedFromTiva(uint8_t);
    void proximityAlarmReceivedFromTiva();
    void fifoReceivedFromTiva(uint8_t* fifo, uint8_t size);

public slots:
    void startRPCClient(QString puerto); //Este Slot arranca la comunicacion
    void ping(void); //Este Slot provoca el envio del comando PING
    void LEDGpio(bool red, bool green, bool blue); //Este Slot provoca el envio del comando LED
    void LEDPwmBrightness(double value); //Este Slot provoca el envio del comando Brightness
    void LEDPwmColor(uint8_t red, uint8_t green, uint8_t blue); // Este Slot provoca el envío de color
    void buttonsRequest();
    void samplingConfig(bool active, bool mode12, int rate);
    void colorRequest();
    void configThreshold(int threshold);
    void configSensorMode(bool fifo_mode);

private slots:
    void processIncommingSerialData(); //Este Slot se conecta a la señal readyRead(..) del puerto serie. Se encarga de procesar y decodificar los mensajes que llegan de la TIVA y
                        //generar señales para algunos de ellos.

private:
    QSerialPort serial;
    QString LastError;
    bool connected;
    QByteArray incommingDataBuffer;

};

#endif // QTIVARPC_H
