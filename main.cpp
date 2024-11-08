#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <stdexcept>

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BMPInfoHeader {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

struct Pixel {
    uint8_t rgbtBlue;
    uint8_t rgbtGreen;
    uint8_t rgbtRed;
};

class BMP {
public:
    BMP(const std::string &filename);
    ~BMP() = default;
    void Save(const std::string &filename);
    void Rotate90();
    void RotateCounter90();
    void GaussianFilter();

private:
    BMPHeader header;
    BMPInfoHeader infoHeader;
    std::vector<std::vector<Pixel>> data; 
};

BMP::BMP(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Error opening file.");
    }

    file.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (header.bfType != 0x4D42) {
        throw std::runtime_error("File format is not BMP.");
    }

    file.read(reinterpret_cast<char *>(&infoHeader), sizeof(infoHeader));

    infoHeader.biHeight = std::abs(infoHeader.biHeight);
    infoHeader.biWidth = std::abs(infoHeader.biWidth);

    if (infoHeader.biWidth == 0 || infoHeader.biHeight == 0) {
        throw std::runtime_error("Unacceptable file size.");
    }
    
    file.seekg(header.bfOffBits, file.beg);

    data.resize(infoHeader.biHeight, std::vector<Pixel>(infoHeader.biWidth));

    for (int i = 0; i < infoHeader.biHeight; ++i) {
        for (int j = 0; j < infoHeader.biWidth; ++j) {
            file.read(reinterpret_cast<char *>(&data[i][j]), sizeof(Pixel));
            if (!file) {
                throw std::runtime_error("Error reading file.");
            }
        }
    }

    file.close();
}

void BMP::Save(const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("File save error.");
    }

    int byte = infoHeader.biHeight * infoHeader.biWidth * sizeof(Pixel);

    std::cout << "File " << filename << " uses " << byte << " bytes." << std::endl;

    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(reinterpret_cast<const char *>(&infoHeader), sizeof(infoHeader));

    for (int i = 0; i < infoHeader.biHeight; ++i) {
        for (int j = 0; j < infoHeader.biWidth; ++j) {
            file.write(reinterpret_cast<const char *>(&data[i][j]), sizeof(Pixel));
        }
    }
    
    file.close();
}

void BMP::Rotate90() {
    std::vector<std::vector<Pixel>> rotatedData(infoHeader.biWidth, std::vector<Pixel>(infoHeader.biHeight));

    for (int i = 0; i < infoHeader.biHeight; ++i) {
        for (int j = 0; j < infoHeader.biWidth; ++j) {
            rotatedData[j][infoHeader.biHeight - i - 1] = data[i][j];
        }
    }

   data = rotatedData;

   std::swap(infoHeader.biWidth, infoHeader.biHeight);
}

void BMP::RotateCounter90() {
   std::vector<std::vector<Pixel>> rotatedData(infoHeader.biWidth, std::vector<Pixel>(infoHeader.biHeight));

   for (int i = 0; i < infoHeader.biHeight; ++i) {
       for (int j = 0; j < infoHeader.biWidth; ++j) {
           rotatedData[infoHeader.biWidth - j - 1][i] = data[i][j];
       }
   }

   data = rotatedData;

   std::swap(infoHeader.biWidth, infoHeader.biHeight);
}

void BMP::GaussianFilter() {
   float kernel[3][3] = {
       {1 / 16.0f, 2 / 16.0f, 1 / 16.0f},
       {2 / 16.0f, 4 / 16.0f, 2 / 16.0f},
       {1 / 16.0f, 2 / 16.0f, 1 / 16.0f}
   };

   std::vector<std::vector<Pixel>> tempData(infoHeader.biHeight, std::vector<Pixel>(infoHeader.biWidth));

   for (int y = 0; y < infoHeader.biHeight; ++y) {
       for (int x = 0; x < infoHeader.biWidth; ++x) {
           float sumRed = 0, sumGreen = 0, sumBlue = 0;

           for (int ky = -1; ky <= 1; ++ky) {
               for (int kx = -1; kx <= 1; ++kx) {
                   int nx = x + kx;
                   int ny = y + ky;

                   nx = std::max(0, std::min(nx, infoHeader.biWidth - 1));
                   ny = std::max(0, std::min(ny, infoHeader.biHeight - 1));

                   sumRed   += data[ny][nx].rgbtRed * kernel[ky + 1][kx + 1];
                   sumGreen += data[ny][nx].rgbtGreen * kernel[ky + 1][kx + 1];
                   sumBlue  += data[ny][nx].rgbtBlue * kernel[ky + 1][kx + 1];
               }
           }
           tempData[y][x].rgbtRed   = static_cast<uint8_t>(sumRed < 0 ? 0 : (sumRed > 255 ? 255 : sumRed));
           tempData[y][x].rgbtGreen = static_cast<uint8_t>(sumGreen < 0 ? 0 : (sumGreen > 255 ? 255 : sumGreen));
           tempData[y][x].rgbtBlue  = static_cast<uint8_t>(sumBlue < 0 ? 0 : (sumBlue > 255 ? 255 : sumBlue));
       }
   }

   data = tempData;
}

int main() {
   try {
       BMP image("yard.bmp");
       BMP imageCounterClockwise("yard.bmp");

       image.Rotate90();
       image.Save("output_clockwise.bmp");

       imageCounterClockwise.RotateCounter90();
       imageCounterClockwise.Save("output_counterclockwise.bmp");

       image.GaussianFilter();
       image.Save("output_gaussian.bmp");

   } catch (const std::exception &e) {
       std::cerr << "Error: " << e.what() << std::endl;
   }
   return 0;
}
