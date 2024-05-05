class GainReductionMeter : public juce::Component
{
public:
    GainReductionMeter() : gainReductionDb(0.0f), smoothedGainReductionDb(0.0f) 
    {
    }

    void paint(juce::Graphics& g) override {
       // g.fillAll(juce::Colours::black);
        auto bounds = getLocalBounds().toFloat().reduced(10);
        auto centre = bounds.getCentre();
        //float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2;
        float radius = bounds.getWidth() / 2;
        drawScale(g, centre, radius);
        drawNeedle(g, centre, radius, smoothedGainReductionDb);
    }

    void drawScale(juce::Graphics& g, juce::Point<float> centre, float radius) {
        const int numMarks = 21;  // Derecelendirme çizgilerinin sayısı
        g.setColour(juce::Colours::white);  // Çizgilerin rengi

        for (int i = 0; i < numMarks; ++i) {
            // Derece çizgilerinin açısını hesapla
            float angle = juce::MathConstants<float>::pi * (1.75f + 0.5f * (i / (numMarks - 1.0f)));

            // Derece çizgilerinin başlangıç ve bitiş noktalarını daha içe çekerek hesapla
            juce::Point<float> end = centre.getPointOnCircumference(radius - 10, angle);
            juce::Point<float> start = centre.getPointOnCircumference(radius - 20, angle);

            // Çizgileri çiz
            g.drawLine(juce::Line<float>(start, end), (i % 5 == 0) ? 3.0f : 1.5f);

            if (i % 5 == 0) { // Büyük işaretler için sayısal değerleri ekle
                auto text = juce::String(-20 + i);
                juce::Rectangle<float> textArea(0, 0, 40, 20);
                // Text alanını çizgiden daha içeri çekerek ayarla
                textArea.setCentre(start.getPointOnCircumference(-15, angle));
                g.drawText(text, textArea, juce::Justification::centred);
            }
        }
    }



    void drawNeedle(juce::Graphics& g, juce::Point<float> centre, float radius, float value) {
        if(value <= 20.0f) {
            float angle = juce::jmap(value, 0.0f, maxGainReduction, juce::MathConstants<float>::pi * 2.25f, juce::MathConstants<float>::pi * 1.75f);
            juce::Point<float> needleEnd = centre.getPointOnCircumference(radius - 20, angle);
            g.setColour(juce::Colours::red);
            g.drawLine(juce::Line<float>(centre, needleEnd), 3.0f);
        } else {
            g.setColour(juce::Colours::red);
            g.drawLine(juce::Line<float>(centre, centre.getPointOnCircumference(radius - 20, juce::MathConstants<float>::pi * 1.75f)), 3.0f);
        }
    }

    void setGainReductionDb(float reductionDb) {
        gainReductionDb = reductionDb;
        smoothedGainReductionDb += (gainReductionDb - smoothedGainReductionDb) * smoothingFactor;
        repaint();
    }

private:
    float gainReductionDb;
    float smoothedGainReductionDb;
    static constexpr float maxGainReduction = 20.0f; // Maksimum dB azalma
    static constexpr float smoothingFactor = 0.1f; // Yumuşatma faktörü
};
