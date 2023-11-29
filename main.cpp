#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static const double c_pi =          3.14159265359;
static const double c_goldenRatio = 1.61803398875;
static const double c_e =           2.71828182845;

static const double c_goldenRatioConjugate = 0.61803398875;

// when making a continued fraction, if the fractional remainder part is less than this, consider it zero.
static const double c_zeroThreshold = 0.000001; // 1 / 1 million

struct LabelAndNumber
{
    const char* label;
    double number;
};

std::vector<int> ToContinuedFraction(double f, int maxContinuedFractionTerms = 20)
{
    std::vector<int> continuedFraction;

    while (maxContinuedFractionTerms == 0 || int(continuedFraction.size()) < maxContinuedFractionTerms)
    {
        // break the number into the integer and fractional part.
        int integerPart = int(f);
        double fractionalPart = f - floor(f);

        // the integer part is the next number in the continued fraction
        continuedFraction.push_back(integerPart);

        // if no fractional part, we are done
        if (fractionalPart < c_zeroThreshold)
            break;

        // f = 1/fractionalPart and continue
        f = 1.0 / fractionalPart;
    }

    return continuedFraction;
}

double FromContinuedFraction(const std::vector<int>& continuedFraction, int count = 0)
{
    double ret = 0.0;
    if (count == 0)
        count = (int)continuedFraction.size();
    int index = std::min(count, (int)continuedFraction.size()) - 1;
    for (; index >= 0; --index)
    {
        if (ret != 0.0)
            ret = 1.0 / ret;
        ret += continuedFraction[index];
    }
    return ret;
}

void ToFraction(const std::vector<int>& continuedFraction, int count, size_t& numerator, size_t& denominator)
{
    numerator = 0;
    denominator = 1;

    if (count == 0)
        count = (int)continuedFraction.size();
    int index = std::min(count, (int)continuedFraction.size()) - 1;

    for (; index >= 0; --index)
    {
        if (numerator != 0)
            std::swap(numerator, denominator);
        numerator += size_t(continuedFraction[index]) * denominator;
    }

}

void PrintContinuedFraction(double f, const char* label = nullptr, int maxContinuedFractionTerms = 20)
{
    std::vector<int> cf = ToContinuedFraction(f, maxContinuedFractionTerms);
    if (label)
        printf("%s = %f = [%i", label, f,  cf[0]);
    else
        printf("%f = [%i", f, cf[0]);
    for (size_t index = 1; index < cf.size(); ++index)
        printf(", %i", cf[index]);
    printf("]\n");
}

void Test_ContinuedFractionError(const char* fileName, const std::vector<LabelAndNumber>& labelsAndNumbers)
{
    FILE* file = nullptr;
    fopen_s(&file, fileName, "w+t");

    for (const LabelAndNumber& labelAndNumber : labelsAndNumbers)
    {
        fprintf(file, "\"%s\"", labelAndNumber.label);

        std::vector<int> continuedFraction = ToContinuedFraction(labelAndNumber.number);

        for (int digits = 1; digits < continuedFraction.size(); ++digits)
        {
            double value = FromContinuedFraction(continuedFraction, digits);
            double relativeError = value / labelAndNumber.number - 1.0;
            fprintf(file, ",\"%f\"", abs(relativeError));
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

// -------------------------------------------------------------------------------

struct RGB
{
    unsigned char R, G, B;
};

float SmoothStep(float value, float min, float max)
{
    float x = (value - min) / (max - min);
    x = std::min(x, 1.0f);
    x = std::max(x, 0.0f);

    return 3.0f * x * x - 2.0f * x * x * x;
}

template <typename T>
T Lerp(T A, T B, float t)
{
    return T(float(A) * (1.0f - t) + float(B) * t);
}

void DrawLine(std::vector<RGB>& image, int width, int height, int x1, int y1, int x2, int y2, RGB color)
{
    // pad the AABB of pixels we scan, to account for anti aliasing
    int startX = std::max(std::min(x1, x2) - 4, 0);
    int startY = std::max(std::min(y1, y2) - 4, 0);
    int endX = std::min(std::max(x1, x2) + 4, width - 1);
    int endY = std::min(std::max(y1, y2) + 4, height - 1);

    // if (x1,y1) is A and (x2,y2) is B, get a normalized vector from A to B called AB
    float ABX = float(x2 - x1);
    float ABY = float(y2 - y1);
    float ABLen = std::sqrt(ABX*ABX + ABY * ABY);
    ABX /= ABLen;
    ABY /= ABLen;

    // scan the AABB of our line segment, drawing pixels for the line, as is appropriate
    for (int iy = startY; iy <= endY; ++iy)
    {
        RGB* pixel = &image[(iy * width + startX)];
        for (int ix = startX; ix <= endX; ++ix)
        {
            // project this current pixel onto the line segment to get the closest point on the line segment to the point
            float ACX = float(ix - x1);
            float ACY = float(iy - y1);
            float lineSegmentT = ACX * ABX + ACY * ABY;
            lineSegmentT = std::min(lineSegmentT, ABLen);
            lineSegmentT = std::max(lineSegmentT, 0.0f);
            float closestX = float(x1) + lineSegmentT * ABX;
            float closestY = float(y1) + lineSegmentT * ABY;

            // calculate the distance from this pixel to the closest point on the line segment
            float distanceX = float(ix) - closestX;
            float distanceY = float(iy) - closestY;
            float distance = std::sqrtf(distanceX*distanceX + distanceY * distanceY);

            // use the distance to figure out how transparent the pixel should be, and apply the color to the pixel
            float alpha = SmoothStep(distance, 2.0f, 0.0f);

            if (alpha > 0.0f)
            {
                pixel->R = Lerp(pixel->R, color.R, alpha);
                pixel->G = Lerp(pixel->G, color.G, alpha);
                pixel->B = Lerp(pixel->B, color.B, alpha);
            }

            pixel++;
        }
    }
}

void DrawCircleFilled(std::vector<RGB>& image, int width, int height, int cx, int cy, int radius, RGB color)
{
    int startX = std::max(cx - radius - 4, 0);
    int startY = std::max(cy - radius - 4, 0);
    int endX = std::min(cx + radius + 4, width - 1);
    int endY = std::min(cy + radius + 4, height - 1);

    for (int iy = startY; iy <= endY; ++iy)
    {
        float dy = float(cy - iy);
        RGB* pixel = &image[(iy * width + startX)];
        for (int ix = startX; ix <= endX; ++ix)
        {
            float dx = float(cx - ix);

            float distance = std::max(std::sqrtf(dx * dx + dy * dy) - float(radius), 0.0f);

            float alpha = SmoothStep(distance, 2.0f, 0.0f);

            if (alpha > 0.0f)
            {
                pixel->R = Lerp(pixel->R, color.R, alpha);
                pixel->G = Lerp(pixel->G, color.G, alpha);
                pixel->B = Lerp(pixel->B, color.B, alpha);
            }

            pixel ++;
        }
    }
}

void DrawCircle(std::vector<RGB>& image, int width, int height, int cx, int cy, int radius, RGB color)
{
    int startX = std::max(cx - radius - 4, 0);
    int startY = std::max(cy - radius - 4, 0);
    int endX = std::min(cx + radius + 4, width - 1);
    int endY = std::min(cy + radius + 4, height - 1);

    for (int iy = startY; iy <= endY; ++iy)
    {
        float dy = float(cy - iy);
        RGB* pixel = &image[(iy * width + startX)];
        for (int ix = startX; ix <= endX; ++ix)
        {
            float dx = float(cx - ix);

            float distance = abs(std::sqrtf(dx * dx + dy * dy) - float(radius));

            float alpha = SmoothStep(distance, 2.0f, 0.0f);

            if (alpha > 0.0f)
            {
                pixel->R = Lerp(pixel->R, color.R, alpha);
                pixel->G = Lerp(pixel->G, color.G, alpha);
                pixel->B = Lerp(pixel->B, color.B, alpha);
            }

            pixel++;
        }
    }
}

float Fract(float x)
{
    return x - floor(x);
}

void NumberlineAndCircleTestBN(const char* baseFileName)
{
    static const int c_numFrames = 16;

    static const int c_circleImageSize = 256;
    static const int c_circleRadius = 120;

    static const int c_numberlineImageWidth = c_circleImageSize;
    static const int c_numberlineImageHeight = c_numberlineImageWidth / 4;
    static const int c_numberlineStartX = c_numberlineImageWidth / 10;
    static const int c_numberlineSizeX = c_numberlineImageWidth * 8 / 10;
    static const int c_numberlineEndX = c_numberlineStartX + c_numberlineSizeX;
    static const int c_numberlineLineStartY = (c_numberlineImageHeight / 2) - 10;
    static const int c_numberlineLineEndY = (c_numberlineImageHeight / 2) + 10;

    std::mt19937 rng(0x1337beef);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    std::vector<float> values;
    char fileName[256];
    for (int frame = 0; frame < c_numFrames; ++frame)
    {
        // Mitchell's best candidate
        if (frame > 0)
        {
            int candidateCount = frame + 1;
            float bestCandidate = 0.0f;
            float bestCandidateScore = -1.0f;
            for (int candidateIndex = 0; candidateIndex < candidateCount; ++candidateIndex)
            {
                float candidate = dist(rng);

                float minDist = FLT_MAX;
                for (float f : values)
                {
                    float dist = std::abs(f - candidate);
                    if (dist > 0.5f)
                        dist = 1.0f - dist;
                    minDist = std::min(minDist, dist);
                }

                if (minDist > bestCandidateScore)
                {
                    bestCandidateScore = minDist;
                    bestCandidate = candidate;
                }
            }
            values.push_back(bestCandidate);
        }
        else
            values.push_back(0.0f);

        std::vector<RGB> circleImageLeft(c_circleImageSize * c_circleImageSize, RGB{ 255,255,255 });
        std::vector<RGB> circleImageRight(c_circleImageSize * c_circleImageSize, RGB{ 255,255,255 });
        std::vector<RGB> numberlineImageLeft(c_numberlineImageWidth * c_numberlineImageHeight, RGB{ 255, 255, 255 });
        std::vector<RGB> numberlineImageRight(c_numberlineImageWidth * c_numberlineImageHeight, RGB{ 255, 255, 255 });

        DrawCircle(circleImageLeft, c_circleImageSize, c_circleImageSize, 128, 128, c_circleRadius, RGB{ 0,0,0 });
        DrawCircle(circleImageRight, c_circleImageSize, c_circleImageSize, 128, 128, c_circleRadius, RGB{ 0,0,0 });

        DrawLine(numberlineImageLeft, c_numberlineImageWidth, c_numberlineImageHeight, c_numberlineStartX, c_numberlineImageHeight / 2, c_numberlineEndX, c_numberlineImageHeight / 2, RGB{ 0, 0, 0 });
        DrawLine(numberlineImageRight, c_numberlineImageWidth, c_numberlineImageHeight, c_numberlineStartX, c_numberlineImageHeight / 2, c_numberlineEndX, c_numberlineImageHeight / 2, RGB{ 0, 0, 0 });

        for (int sample = 0; sample <= frame; ++sample)
        {
            float value = values[sample];

            float angle = value * (float)c_pi * 2.0f;

            int targetX = int(cos(angle) * float(c_circleRadius)) + 128;
            int targetY = int(sin(angle) * float(c_circleRadius)) + 128;

            unsigned char percentColor = (unsigned char)(255.0f - 255.0f * float(sample) / float(c_numFrames - 1));

            RGB sampleColor = (sample == frame) ? RGB{ 255, 0, 0 } : RGB{ 192, percentColor, 0 };

            DrawLine(circleImageLeft, c_circleImageSize, c_circleImageSize, 128, 128, targetX, targetY, sampleColor);

            if (sample >= c_numFrames / 2)
                DrawLine(circleImageRight, c_circleImageSize, c_circleImageSize, 128, 128, targetX, targetY, sampleColor);

            targetX = int(value * float(c_numberlineSizeX)) + c_numberlineStartX;
            DrawLine(numberlineImageLeft, c_numberlineImageWidth, c_numberlineImageHeight, targetX, c_numberlineLineStartY, targetX, c_numberlineLineEndY, sampleColor);

            if (sample >= c_numFrames / 2)
                DrawLine(numberlineImageRight, c_numberlineImageWidth, c_numberlineImageHeight, targetX, c_numberlineLineStartY, targetX, c_numberlineLineEndY, sampleColor);
        }

        int outImageW = c_circleImageSize * 2;
        int outImageH = c_circleImageSize + c_numberlineImageHeight;
        std::vector<RGB> outputImage(outImageW * outImageH);

        RGB* dest = outputImage.data();
        const RGB* srcLeft = circleImageLeft.data();
        const RGB* srcRight = circleImageRight.data();
        for (int i = 0; i < c_circleImageSize; ++i)
        {
            memcpy(dest, srcLeft, c_circleImageSize * 3);
            dest += c_circleImageSize;
            srcLeft += c_circleImageSize;
            memcpy(dest, srcRight, c_circleImageSize * 3);
            dest += c_circleImageSize;
            srcRight += c_circleImageSize;
        }

        srcLeft = numberlineImageLeft.data();
        srcRight = numberlineImageRight.data();
        for (int i = 0; i < c_numberlineImageHeight; ++i)
        {
            memcpy(dest, srcLeft, c_circleImageSize * 3);
            dest += c_circleImageSize;
            srcLeft += c_circleImageSize;
            memcpy(dest, srcRight, c_circleImageSize * 3);
            dest += c_circleImageSize;
            srcRight += c_circleImageSize;
        }

        sprintf_s(fileName, "out/%s_%i.png", baseFileName, frame);
        stbi_write_png(fileName, outImageW, outImageH, 3, outputImage.data(), outImageW * 3);
    }
}

void NumberlineAndCircleTest(const char* baseFileName, float irrational)
{
    static const int c_numFrames = 16;

    static const int c_circleImageSize = 256;
    static const int c_circleRadius = 120;

    static const int c_numberlineImageWidth = c_circleImageSize;
    static const int c_numberlineImageHeight = c_numberlineImageWidth/4;
    static const int c_numberlineStartX = c_numberlineImageWidth / 10;
    static const int c_numberlineSizeX = c_numberlineImageWidth * 8 / 10;
    static const int c_numberlineEndX = c_numberlineStartX + c_numberlineSizeX;
    static const int c_numberlineLineStartY = (c_numberlineImageHeight / 2) - 10;
    static const int c_numberlineLineEndY = (c_numberlineImageHeight / 2) + 10;

    char fileName[256];
    for (int frame = 0; frame < c_numFrames; ++frame)
    {
        std::vector<RGB> circleImageLeft(c_circleImageSize*c_circleImageSize, RGB{ 255,255,255 });
        std::vector<RGB> circleImageRight(c_circleImageSize*c_circleImageSize, RGB{ 255,255,255 });
        std::vector<RGB> numberlineImageLeft(c_numberlineImageWidth*c_numberlineImageHeight, RGB{ 255, 255, 255 });
        std::vector<RGB> numberlineImageRight(c_numberlineImageWidth*c_numberlineImageHeight, RGB{ 255, 255, 255 });

        DrawCircle(circleImageLeft, c_circleImageSize, c_circleImageSize, 128, 128, c_circleRadius, RGB{ 0,0,0 });
        DrawCircle(circleImageRight, c_circleImageSize, c_circleImageSize, 128, 128, c_circleRadius, RGB{ 0,0,0 });

        DrawLine(numberlineImageLeft, c_numberlineImageWidth, c_numberlineImageHeight, c_numberlineStartX, c_numberlineImageHeight / 2, c_numberlineEndX, c_numberlineImageHeight / 2, RGB{0, 0, 0});
        DrawLine(numberlineImageRight, c_numberlineImageWidth, c_numberlineImageHeight, c_numberlineStartX, c_numberlineImageHeight / 2, c_numberlineEndX, c_numberlineImageHeight / 2, RGB{ 0, 0, 0 });

        float value = 0.0f;
        for (int sample = 0; sample <= frame; ++sample)
        {
            float angle = value * (float)c_pi * 2.0f;

            int targetX = int(cos(angle) * float(c_circleRadius)) + 128;
            int targetY = int(sin(angle) * float(c_circleRadius)) + 128;

            unsigned char percentColor = (unsigned char)(255.0f - 255.0f * float(sample) / float(c_numFrames-1));

            RGB sampleColor = (sample == frame) ? RGB{ 255, 0, 0 } : RGB{ 192, percentColor, 0 };

            DrawLine(circleImageLeft, c_circleImageSize, c_circleImageSize, 128, 128, targetX, targetY, sampleColor);

            if (sample >= c_numFrames / 2)
                DrawLine(circleImageRight, c_circleImageSize, c_circleImageSize, 128, 128, targetX, targetY, sampleColor);

            targetX = int(value * float(c_numberlineSizeX)) + c_numberlineStartX;
            DrawLine(numberlineImageLeft, c_numberlineImageWidth, c_numberlineImageHeight, targetX, c_numberlineLineStartY, targetX, c_numberlineLineEndY, sampleColor);

            if (sample >= c_numFrames / 2)
                DrawLine(numberlineImageRight, c_numberlineImageWidth, c_numberlineImageHeight, targetX, c_numberlineLineStartY, targetX, c_numberlineLineEndY, sampleColor);

            value = Fract(value + irrational);
        }

        int outImageW = c_circleImageSize * 2;
        int outImageH = c_circleImageSize + c_numberlineImageHeight;
        std::vector<RGB> outputImage(outImageW*outImageH);

        RGB* dest = outputImage.data();
        const RGB* srcLeft = circleImageLeft.data();
        const RGB* srcRight = circleImageRight.data();
        for (int i = 0; i < c_circleImageSize; ++i)
        {
            memcpy(dest, srcLeft, c_circleImageSize * 3);
            dest += c_circleImageSize;
            srcLeft += c_circleImageSize;
            memcpy(dest, srcRight, c_circleImageSize * 3);
            dest += c_circleImageSize;
            srcRight += c_circleImageSize;
        }

        srcLeft = numberlineImageLeft.data();
        srcRight = numberlineImageRight.data();
        for (int i = 0; i < c_numberlineImageHeight; ++i)
        {
            memcpy(dest, srcLeft, c_circleImageSize * 3);
            dest += c_circleImageSize;
            srcLeft += c_circleImageSize;
            memcpy(dest, srcRight, c_circleImageSize * 3);
            dest += c_circleImageSize;
            srcRight += c_circleImageSize;
        }

        sprintf_s(fileName, "out/%s_%i.png", baseFileName, frame);
        stbi_write_png(fileName, outImageW, outImageH, 3, outputImage.data(), outImageW * 3);
    }
}

int main(int argc, char** argv)
{
    NumberlineAndCircleTestBN("blue");
    NumberlineAndCircleTest("golden", (float)c_goldenRatioConjugate);
    NumberlineAndCircleTest("pi", (float)c_pi);
    NumberlineAndCircleTest("sqrt2", sqrt(2.0f));

    return 0;

    // Show some continued fractions
    {
        printf("Continued Fractions...\n");

        PrintContinuedFraction(0.0);
        PrintContinuedFraction(1.0);
        PrintContinuedFraction(8.25);
        PrintContinuedFraction(4.1);

        PrintContinuedFraction(c_pi, "Pi");

        PrintContinuedFraction(c_goldenRatio, "Golden Ratio");
        PrintContinuedFraction(c_goldenRatio - 1.0, "Golden Ratio Conjugate");

        PrintContinuedFraction(c_e, "e");

        PrintContinuedFraction(sqrt(2.0), "sqrt(2)");

        PrintContinuedFraction(sqrt(3.0), "sqrt(3)");

        PrintContinuedFraction(sqrt(5.0), "sqrt(5)");

        PrintContinuedFraction(sqrt(7.0), "sqrt(7)");
    }

    // show the evolution of evaluating a continued fraction - pi
    {
        printf("\n\nShowing evaluation of continued fraction of pi (%f)...\n", c_pi);
        std::vector<int> CF = ToContinuedFraction(c_pi);

        for (size_t i = 1; i < CF.size(); ++i)
        {
            size_t n, d;
            ToFraction(CF, (int)i, n, d);
            double value = FromContinuedFraction(CF, (int)i);
            double relativeError = abs(value / c_pi - 1.0);
            printf("[%i] %f aka %zu/%zu (%f)\n", CF[i-1], value, n, d, relativeError);
        }
    }

    // show the evolution of evaluating a continued fraction - golden ratio
    {
        printf("\n\nShowing evaluation of continued fraction of golden ratio (%f)...\n", c_goldenRatio);
        std::vector<int> CF = ToContinuedFraction(c_goldenRatio);

        for (size_t i = 1; i < CF.size(); ++i)
        {
            size_t n, d;
            ToFraction(CF, (int)i, n, d);
            double value = FromContinuedFraction(CF, (int)i);
            double relativeError = abs(value / c_goldenRatio - 1.0);
            printf("[%i] %f aka %zu/%zu (%f)\n", CF[i - 1], value, n, d, relativeError);
        }
    }

    // show the evolution of evaluating a continued fraction - golden ratio conjugate
    {
        printf("\n\nShowing evaluation of continued fraction of golden ratio conjugate (%f)...\n", c_goldenRatioConjugate);
        std::vector<int> CF = ToContinuedFraction(c_goldenRatioConjugate);

        for (size_t i = 1; i < CF.size(); ++i)
        {
            size_t n, d;
            ToFraction(CF, (int)i, n, d);
            double value = FromContinuedFraction(CF, (int)i);
            double relativeError = abs(value / c_goldenRatioConjugate - 1.0);
            printf("[%i] %f aka %zu/%zu (%f)\n", CF[i - 1], value, n, d, relativeError);
        }
    }

    // show some numbers made from continued fractions
    {
        printf("\n\n");
        Test_ContinuedFractionError("out/cfabsrelerror.csv",
            {
                {"Golden Ratio", c_goldenRatio},
                {"Golden Ratio Conjugate", c_goldenRatio - 1.0},
                {"Pi", c_pi},
                {"Sqrt(2)", sqrt(2.0)},
                {"Sqrt(3)", sqrt(3.0)},
                {"Sqrt(5)", sqrt(5.0)},
                {"Sqrt(7)", sqrt(7.0)},
            }
        );
    }

    // let's make up some numbers and see how they do.
    {
        printf("\n\nMade Up Numbers\n");
        std::vector<int> numberA_CF = { 1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2 };
        std::vector<int> numberB_CF = { 1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,1,2,1,1 };
        std::vector<int> numberC_CF = { 1,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2 };
        double numberA = FromContinuedFraction(numberA_CF);
        double numberB = FromContinuedFraction(numberB_CF);
        double numberC = FromContinuedFraction(numberC_CF);

        // TODO: pass the CF in, instead of remaking it?
        PrintContinuedFraction(numberA, "A");
        PrintContinuedFraction(numberB, "B");
        PrintContinuedFraction(numberC, "C");
        Test_ContinuedFractionError("out/madeup.csv",
            {
                {"Golden Ratio", c_goldenRatio},
                {"Pi", c_pi},
                {"A", numberA},
                {"B", numberB},
                {"C", numberC},
            }
        );
    }

    system("pause");
    return 0;
}

/*

TODO:

I don't really need to link to this code. so, maybe some of these todos can disappear.

*/