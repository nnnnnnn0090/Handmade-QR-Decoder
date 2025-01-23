#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>

using namespace std;

enum class ErrorLevel {
    L = 0b01,
    M = 0b00,
    Q = 0b11,
    H = 0b10
};

enum class DataMode {
    Numbers = 0b0001,
    Alphanumeric = 0b0010,
    Byte = 0b0100,
    Kanji = 0b1000
};

ostream& operator<<(ostream& os, const ErrorLevel& level) {
    static const map<ErrorLevel, string> map = {
        {ErrorLevel::L, "L"},
        {ErrorLevel::M, "M"},
        {ErrorLevel::Q, "Q"},
        {ErrorLevel::H, "H"}
    };

    auto it = map.find(level);
    if (it != map.end()) {
        os << it->second;
    } else {
        os << "Unknown";
    }
    return os;
}

ostream& operator<<(ostream& os, const DataMode& mode) {
    static const map<DataMode, string> map = {
        {DataMode::Numbers, "Numbers"},
        {DataMode::Alphanumeric, "Alphanumeric"},
        {DataMode::Byte, "Byte"},
        {DataMode::Kanji, "Kanji"}
    };

    auto it = map.find(mode);
    if (it != map.end()) {
        os << it->second;
    } else {
        os << "Unknown";
    }
    return os;
}

class QRCode {
public:
    QRCode() : height(0), width(0), version(0), errorLevel(ErrorLevel::L), maskMode(0) {}

    void printQRCode() const;
    bool checkAlignmentPattern() const;
    void readFormatInfo();
    void decodeData();
    void setupDataMode();
    void printQRInfo() const;
    bool readFromFile(const char* filename);

private:
    vector<vector<char>> qrCode;
    int height;
    int width;
    int version;
    ErrorLevel errorLevel;
    int maskMode;
    vector<int> decodedData;
    DataMode dataMode;
    int length;
    string decodeStr;

    void readErrorLevel(const vector<int>& formatInfo);
    bool isNeedReverse(int i, int j) const;
    bool isNeedSkip(int h, int w) const;
    int decodeLength() const;
    string decodeResult() const;
};

void QRCode::printQRCode() const {
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            if (qrCode[i][j] == '0') {
                cout << " ";
            } else if (qrCode[i][j] == '1') {
                cout << "■";
            } else {
                cout << qrCode[i][j];
            }
            cout << " ";
        }
        cout << endl;
    }
}

bool QRCode::checkAlignmentPattern() const {
    auto check = [&](bool isRow) {
        for (int i = 8; i < (width - 1) - 9; i++) {
            int expected = (i % 2 == 0) ? 1 : 0;
            int actual = (isRow ? qrCode[6][i] : qrCode[i][6]) - '0';
            if (actual != expected) {
                cout << "Mismatch at index " << i << ": expected " << expected << ", found " << actual << endl;
                return false;
            }
        }
        return true;
    };

    if (!check(true) || !check(false)) return false;
    cout << "Alignment pattern is correct." << endl;
    return true;
}

void QRCode::readErrorLevel(const vector<int>& formatInfo) {
    int errorLevelBits = (formatInfo[0] << 1) | formatInfo[1];
    errorLevel = static_cast<ErrorLevel>(errorLevelBits);
    maskMode = (formatInfo[2] << 2) | (formatInfo[3] << 1) | formatInfo[4];
}

bool QRCode::isNeedReverse(int i, int j) const {
    switch (maskMode) {
        case 0: return ((i + j) % 2 == 0);
        case 1: return (i % 2 == 0);
        case 2: return (j % 3 == 0);
        case 3: return ((i + j) % 3 == 0);
        case 4: return ((i / 2) + (j / 3) % 2 == 0);
        case 5: return ((i * j) % 2 + (i * j) % 3 == 0);
        case 6: return (((i * j) % 2 + (i * j) % 3) % 2 == 0);
        case 7: return (((i * j) % 2 + (i + j) % 2) % 2 == 0);
        default:
            cout << "Invalid mask mode" << endl;
            return false;
    }
}

void QRCode::readFormatInfo() {
    vector<int> formatInfo(15);
    vector<int> xorKey = {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0};

    int index = 0;
    for (int i = 0; i <= 8; i++) {
        if (i == 6) continue;
        formatInfo[index++] = qrCode[i][8] - '0';
    }
    for (int i = (height - 1) - 6; i <= height - 1; i++) {
        formatInfo[index++] = qrCode[i][8] - '0';
    }

    reverse(formatInfo.begin(), formatInfo.end());

    for (int i = 0; i < 15; i++) {
        formatInfo[i] ^= xorKey[i];
    }

    cout << "After XOR: ";
    for (int val : formatInfo) {
        cout << val << " ";
    }
    cout << endl;

    readErrorLevel(formatInfo);
}

bool QRCode::isNeedSkip(int h, int w) const {
    if ((width - 1) - 7 <= w && h <= 8) return true;
    if ((width - 1) - 8 <= w && w <= (width - 1) - 4 && (height - 1) - 8 <= h && h <= (height - 1) - 4) return true;
    if (h == 6 || w == 6) return true;
    if (w <= 8 && h <= 8) return true;
    if (w <= 8 && (height - 1) - 7 <= h) return true;
    return false;
}

void QRCode::decodeData() {
    int index = 0;
    decodedData.clear();

    for (int w = (width - 1); w >= 2; w -= 2) {
        if (w % 4 == 0) {
            for (int h = (height - 1); h >= 0; h--) {
                for (int k = 0; k < 2; k++) {
                    if (isNeedSkip(h, w - k)) continue;
                    int num = qrCode[h][w - k] - '0';
                    if (isNeedReverse(h, w - k)) num ^= 1;
                    decodedData.push_back(num);
                }
            }
        } else {
            for (int h = 0; h < height; h++) {
                for (int k = 0; k < 2; k++) {
                    if (isNeedSkip(h, w - k)) continue;
                    int num = qrCode[h][w - k] - '0';
                    if (isNeedReverse(h, w - k)) num ^= 1;
                    decodedData.push_back(num);
                }
            }
        }

        if (w == 8) w--;
    }

    decodedData.push_back(2); // End marker

    setupDataMode();
    length = decodeLength();
    decodeStr = decodeResult();
}

int QRCode::decodeLength() const {
    if (decodedData.size() < 12) {
        cerr << "Error: Insufficient data to determine length." << endl;
        return 0;
    }

    int length = 0;
    for (int i = 0; i < 8; ++i) {
        length = (length << 1) | decodedData[4 + i];
    }
    return length;
}

string QRCode::decodeResult() const {
    if (decodedData.size() < 12 + (length * 8)) {
        cerr << "Error: Insufficient data to decode result string." << endl;
        return "";
    }

    string result;
    for (int i = 12; i < (length * 8) + 12; i += 8) {
        char character = 0;
        for (int j = 0; j < 8; ++j) {
            character = (character << 1) | decodedData[i + j];
        }
        result.push_back(character);
    }
    return result;
}

void QRCode::printQRInfo() const {
    checkAlignmentPattern();
    const_cast<QRCode*>(this)->readFormatInfo();

    cout << "Version: " << version << endl;
    cout << "Width  : " << width << endl;
    cout << "Height : " << height << endl;
    cout << "ErrorLevel : " << errorLevel << endl;
    cout << "MaskMode : " << maskMode << endl;

    const_cast<QRCode*>(this)->decodeData();

    cout << "DataMode : " << dataMode << endl;

    cout << "Length : " << length << endl;

    cout << "Resulting String : " << decodeStr << endl;
}

void QRCode::setupDataMode() {
    if (decodedData.size() < 4) {
        cerr << "Error: Insufficient data to determine data mode." << endl;
        dataMode = DataMode::Numbers;
        return;
    }

    dataMode = static_cast<DataMode>((decodedData[0] << 3) | (decodedData[1] << 2) | (decodedData[2] << 1) | decodedData[3]);
}

bool QRCode::readFromFile(const char* filename) {
    const int MAX_COLS = 100;
    height = 0;

    ifstream file(filename);
    if (!file) {
        cerr << "Error: Unable to open file " << filename << endl;
        return false;
    }

    string line;
    while (getline(file, line)) {
        vector<char> row;
        for (char c : line) {
            if (c != ' ') row.push_back(c);
        }
        if (!row.empty()) {
            qrCode.push_back(row);
            height++;
        }
    }

    if (!qrCode.empty()) {
        width = qrCode[0].size();
        version = ((width - 21) / 4) + 1;
    }

    file.close();
    return true;
}

int main() {
    QRCode qr;

    if (!qr.readFromFile("qr.txt")) {
        return 1;
    }

    qr.printQRCode();
    qr.printQRInfo();

    return 0;
}
