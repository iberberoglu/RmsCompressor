#include "JuceHeader.h"

class GainReductionMeter : public juce::Component
{
public:
    GainReductionMeter() : gainReductionDb(0.0f), smoothedGainReductionDb(0.0f)
    {
    }

    void paint(juce::Graphics& g) override {
        
        auto bounds = getLocalBounds().toFloat().reduced(10);
        auto centre = bounds.getCentre();
        float radius = bounds.getWidth() / 2;

        drawScale(g, centre, radius);
        drawNeedle(g, centre, radius, smoothedGainReductionDb);
    }

    void drawScale(juce::Graphics& g, juce::Point<float> centre, float radius) {
        const int numMarks = 10; // İşaret sayısı
        const float startAngle = juce::MathConstants<float>::pi * 1.1667f; // Başlangıç açısı, saat 7 yönü (210 derece)
        const float endAngle = juce::MathConstants<float>::pi * 2.8333f; // Bitiş açısı, saat 5 yönü (150 derece)
        std::array<int, numMarks> marks = {0, -5, -10, -15, -20, -30, -40, -50, -60, -80};

        for (int i = 0; i < numMarks; ++i) {
            float angle = juce::jmap(float(i), 0.0f, float(numMarks - 1), startAngle, endAngle);
            float angleSecond = juce::jmap(float(i+1), 0.0f, float(numMarks - 1), startAngle, endAngle);
            juce::Point<float> end = centre.getPointOnCircumference(radius - 10, angle);
            juce::Point<float> start = centre.getPointOnCircumference(radius - 20, angle);

            g.setColour(juce::Colours::white);
            g.drawLine(juce::Line<float>(start, end), 2.0f);
            
            if(i < 4){
                for(int j = 0; j < 5; j++){
                    float angleS = juce::jmap(float(j), 0.0f, float(5), angle, angleSecond);
                    juce::Point<float> endS = centre.getPointOnCircumference(radius - 15, angleS);
                    juce::Point<float> startS = centre.getPointOnCircumference(radius - 20, angleS);
                    g.setColour(juce::Colours::white);
                    g.drawLine(juce::Line<float>(startS, endS), 2.0f);
                }
            }else if(i >= 4 && i < 9){
                for(int j = 0; j < 10; j++){
                    float angleS = juce::jmap(float(j), 0.0f, float(10), angle, angleSecond);
                    juce::Point<float> endS = centre.getPointOnCircumference(radius - 15, angleS);
                    juce::Point<float> startS = centre.getPointOnCircumference(radius - 20, angleS);
                    g.setColour(juce::Colours::white);
                    g.drawLine(juce::Line<float>(startS, endS), 1.0f);
                }
            }


            if (i % 1 == 0) {
                auto text = juce::String(marks[i]);
                juce::Rectangle<float> textArea(0, 0, 40, 20);
                textArea.setCentre(start.getPointOnCircumference(-25, angle));
                g.drawText(text, textArea, juce::Justification::centred);
            }
        }
    }

    void drawNeedle(juce::Graphics& g, juce::Point<float> centre, float radius, float value) {
        float angle;
        if (value <= 20.0f) {
            angle = juce::jmap(value, 0.0f, 20.0f, startAngle, juce::MathConstants<float>::pi * 2.0f);
        } else {
            angle = juce::jmap(value, 20.0f, 80.0f, juce::MathConstants<float>::pi * 2.0f, endAngle);
        }
        juce::Point<float> needleEnd = centre.getPointOnCircumference(radius - 20, angle);
        juce::Point<float> needleStart = centre.getPointOnCircumference(radius - 60, angle);
        g.setColour(juce::Colours::red);
        g.drawLine(juce::Line<float>(needleStart, needleEnd), 2.0f);
    }

    void setGainReductionDb(float reductionDb) {
        gainReductionDb = reductionDb;
        smoothedGainReductionDb += (gainReductionDb - smoothedGainReductionDb) * smoothingFactor;
        repaint();
    }

private:
    float gainReductionDb;
    float smoothedGainReductionDb;
    static constexpr float maxGainReductionLow = 20.0f;
    static constexpr float maxGainReduction = 80.0f;
    static constexpr float smoothingFactor = 0.1f;
    float startAngle = juce::MathConstants<float>::pi * 1.1667f; // Saat 7 yönü
    float endAngle = juce::MathConstants<float>::pi * 2.8333f; // Saat 5 yönü
};
