#include <iostream>
#include "QRCode.h"

int main() {
    QRCode qr;

    if (!qr.loadFromFile("./qr.txt")) {
        return 1;
    }

    qr.printQRCode();
    qr.printQRInfo();

    return 0;
}