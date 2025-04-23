#include <ArduinoEigen.h>
#include <Arduino.h>

using namespace Eigen;

Vector3f xp1;
Vector3f x_offset;
Vector3f gain;
VectorXf a1;
VectorXf b1;
MatrixXf IW1_1;
RowVectorXf LW2_1;
float a2;
float y1_output; 
float b2;
float y_gain;
float y_xoffset;

int IR, umurr, HR, GLU, CHOL;
float ACD;

uint8_t roundUpToUint8(float value);
float roundToOneDecimal(float value);
float myNeuralNetworkFunction(const Eigen::Vector3f& input, int network_id);

void setup() {
    IR = 10000;
    umurr = 30;
    HR = 75;

    Eigen::Vector3f inputData;
    inputData << IR, umurr, HR;
    GLU = roundUpToUint8(myNeuralNetworkFunction(inputData, 1));
    CHOL = roundUpToUint8(myNeuralNetworkFunction(inputData, 2));
    ACD = roundToOneDecimal(myNeuralNetworkFunction(inputData, 3));

    Serial.begin(9600);
    Serial.print("GLU: "); Serial.println(GLU);
    Serial.print("CHOL: "); Serial.println(CHOL);
    Serial.print("ACD: "); Serial.println(ACD);
}

void loop() {
}

uint8_t roundUpToUint8(float value) {
  return static_cast<uint8_t>(ceil(value));
}

float roundToOneDecimal(float value) {
  return round(value * 10.0f) / 10.0f;
}

float myNeuralNetworkFunction(const Eigen::Vector3f& input, int network_id) {
  Eigen::Vector3f x = input;
  Eigen::Vector3f xp1;
  Eigen::VectorXf a1;
  float a2;
  float y1_output;

  switch (network_id) {
    case 1: {
      Eigen::Vector3f x_offset(51735, 9, 26);
      Eigen::Vector3f gain(3.64896916621055e-05, 0.0294117647058824,
                           0.0266666666666667);
      xp1 = (x - x_offset).array() * gain.array() + (-1.0f);

      float b1 = -16.63759766878351698f;
      Eigen::RowVector3f IW1_1(2.1493558295058914354f,
                               14.726331644390356246f,
                               16.513591564909340548f);
      float sum = IW1_1 * xp1 + b1;
      a1.resize(1);
      a1[0] = 2.0f / (1.0f + exp(-2.0f * sum)) - 1.0f;

      float b2 = -0.39083029729430657229f;
      float LW2_1 = 0.24426653523419675218f;
      a2 = LW2_1 * a1[0] + b2;

      float y_gain = 0.0117647058823529f;
      float y_xoffset = 94.0f;
      y1_output = (a2 - (-1.0f)) / y_gain + y_xoffset;
      break;
    }
    case 2: {
      Eigen::Vector3f x_offset(58124, 9, 55);
      Eigen::Vector3f gain(4.34839326868722e-05, 0.0266666666666667,
                           0.0416666666666667);
      xp1 = (x - x_offset).array() * gain.array() + (-1.0f);

      Eigen::VectorXf b1(5);
      b1 << -4.1572857480307350286f, -0.38029472457922613993f,
          0.70156562858405513428f, 1.071440477708899941f,
          2.1105006102053316397f;
      Eigen::MatrixXf IW1_1(5, 3);
      IW1_1 << 2.5350106320688046146f, -3.2503410452606162906f,
          1.6572979890684240711f, 0.90462799086409484417f,
          2.4814449873300388205f, -0.0057440872056210220964f,
          -1.0605734542426203948f, -0.19615437391647000398f,
          1.9414825351178413015f, 1.7987390581184692362f,
          -0.77200529967437936385f, -0.99063946624861731749f,
          0.63577746361881404269f, 2.1635580643615264229f,
          -1.3125931768397518518f;
      Eigen::VectorXf layer1 = IW1_1 * xp1 + b1;
      a1.resize(5);
      for (int i = 0; i < 5; i++) {
        a1[i] = 2.0f / (1.0f + exp(-2.0f * layer1[i])) - 1.0f;
      }

      float b2 = 0.57574514652368069534f;
      Eigen::RowVectorXf LW2_1(5);
      LW2_1 << 2.0816195454248540564f, 0.40146089545829760636f,
          0.24168468981366877935f, -0.0050136968048260094344f,
          1.0409722567092083434f;
      a2 = LW2_1 * a1 + b2;

      float y_gain = 0.3125f;
      float y_xoffset = 2.4f;
      y1_output = (a2 - (-1.0f)) / y_gain + y_xoffset;
      break;
    }
    case 3: {
      Eigen::Vector3f x_offset(52788, 17, 26);
      Eigen::Vector3f gain(3.98374631503466e-05, 0.0298507462686567,
                           0.025974025974026);
      xp1 = (x - x_offset).array() * gain.array() + (-1.0f);

      Eigen::VectorXf b1(3);
      b1 << 2.8987119892188055736f, 9.9275370296609057874f,
          7.3558450309877070339f;
      Eigen::MatrixXf IW1_1(3, 3);
      IW1_1 << 7.8709740143197421958f, 13.886396469436782297f,
          12.231008407705287411f, -13.894948315389491711f,
          -33.600792028840380965f, -15.022515547619278209f,
          0.57951782866092504953f, 5.9329585451750430636f,
          -5.2409751499447558842f;
      Eigen::VectorXf layer1 = IW1_1 * xp1 + b1;
      a1.resize(3);
      for (int i = 0; i < 3; i++) {
        a1[i] = 2.0f / (1.0f + exp(-2.0f * layer1[i])) - 1.0f;
      }

      float b2 = 0.50460391480801192188f;
      Eigen::RowVectorXf LW2_1(3);
      LW2_1 << 0.18972498791585371003f, -0.36183087980935402239f,
          -0.69365338494794059887f;
      a2 = LW2_1 * a1 + b2;

      float y_gain = 0.0105263157894737f;
      float y_xoffset = 100.0f;
      y1_output = (a2 - (-1.0f)) / y_gain + y_xoffset;
      break;
    }
    default:
      return -1.0f;
  }
  return y1_output;
}
