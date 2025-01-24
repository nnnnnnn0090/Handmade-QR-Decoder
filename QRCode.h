#ifndef QR_CODE_H
#define QR_CODE_H

#include <iostream>
#include <vector>
#include <string>
#include <map>

using namespace std;

enum class ErrorLevel {
    M = 0b00,
    L = 0b01,
    H = 0b10,
    Q = 0b11
};

enum class DataMode {
    Unknown = 0b0000,
    Numbers = 0b0001,
    Alphanumeric = 0b0010,
    Byte = 0b0100,
    Kanji = 0b1000
};

class QRInfo {
public:
    ErrorLevel errorLevel;
    int maskMode;

    QRInfo(ErrorLevel errorLevel, int maskMode)
        : errorLevel(errorLevel), maskMode(maskMode) {}
};

ostream& operator<<(ostream& os, const ErrorLevel& level);
ostream& operator<<(ostream& os, const DataMode& mode);

class QRCode {
public:
    QRCode();
    void printQRCode() const;
    void printQRInfo() const;
    bool loadFromFile(const char* filename);

private:
    vector<vector<char>> qrCode;
    int height;
    int width;
    int version;
    QRInfo* info;
    vector<int> decodedData;
    DataMode dataMode;
    int length;
    string decodeStr;

    bool readFromFile(const char* filename);
    QRInfo* readFormatInfo();
    vector<int> readDataBits();
    DataMode readDataMode(vector<int> decodedData);
    bool checkAlignmentPattern() const;
    QRInfo* parseFormatInfo(vector<int> formatInfo);
    bool isNeedReverse(int i, int j) const;
    bool isNeedSkip(int h, int w) const;
    int decodeLength(vector<int> decodedData) const;
    string decodeResult(vector<int> decodedData) const;
};

#endif
