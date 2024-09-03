#ifndef HEARTRATE_H
#define HEARTRATE_H

#include <QLowEnergyService>
#include <QLowEnergyServiceData>
#include <QObject>

class BtService : public QObject {
    Q_OBJECT
public:
    BtService();
    const QLowEnergyServiceData& service() const;
    void run(QLowEnergyService& service);
private:
    QLowEnergyServiceData serviceData;
};
#endif //HEARTRATE_H
