#include <stdio.h>
#include <vector>
#include <algorithm>

static const double c_pi =          3.14159265359;
static const double c_goldenRatio = 1.61803398875;
static const double c_e =           2.71828182845;

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
        if (fractionalPart == 0.0)
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

int main(int argc, char** argv)
{
    // Show some continued fractions
    {
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

    // show some numbers made from continued fractions
    {
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

        // TODO: pass the CF in, instead of remaking it
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
* stuff from email
* try and make some good irrationals like maybe all 2's or have mostly 1's with a 2 every now and then or something
* could try to make a "co-irrational" pair of numbers? or at least show how "co-irrational" they are by dividing by 1 and then looking at continued fraction

Blog:
* explain to and from continued fractions.
* talk about how "bigger numbers earlier mean they are more rational" and look at phi being all 1s.
* could show error as more continued fraction digits are used.
 * nothing converges more slowly than golden ratio. (what about golden ratio conjugate? i don't understand that part. it has higher relative error cause the number is smaller.
 * this is the reason it's called the most irrational - it is the number least well approximated by ratios / rational numbers

*/