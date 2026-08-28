# RMSCompressor

**RMS hesaplama penceresi ve zarf eğrisinin şekli kullanıcı tarafından değiştirilebilen bir kompresör eklentisi.**

`C++` · `JUCE 7.0.12` · `AU` · `VST3` · `macOS`

*[English README →](README.md)*

![Eklenti arayüzü](docs/ui.png)

---

## Nedir

Çoğu kompresör sana attack ve release **süresini** verir. Bu eklenti ayrıca o
sürelerin ürettiği eğrinin **şeklini** de veriyor, ve algılayıcının karar
vermeden önce sesin ne kadarlık bir dilimini ortalayacağını da sana bırakıyor.

Bu ikisi normalde DSP kodunun içine gömülü sabitlerdir. Burada ön panelde
birer kontrol.

Eklenti, İstanbul Teknik Üniversitesi'nde bir lisans dersi projesi olarak
sıfırdan C++ ve JUCE ile yazıldı (bkz. [Proje bağlamı](#proje-bağlamı)).
Aşağıdaki iki fikri mümkün kılmak için JUCE'un iki DSP sınıfı —
`juce::dsp::Compressor` ve `juce::dsp::BallisticsFilter` — yamalandı; yamalı
modüller bu depoda `modules/` altında bulunuyor.

---

## İki fikir

### 1. Değiştirilebilir RMS hesaplama penceresi

JUCE'un `AudioBuffer::getRMSLevel()` metodu, host'un sana verdiği buffer üzerinden
ortalama alır — 128 sample, 512, 1024, DAW ne ayarlıysa o. Bu senin kontrolünde
değil ve kullanıcı buffer boyutunu değiştirdiğinde o da değişir.

Algılayıcıyı host'tan ayırmak için gelen sample'lar kilitsiz (lock-free) bir FIFO
halka tamponuna yazılıyor (`Source/Fifo.h`). RMS, bu halkadan geri çekilen
istenilen uzunlukta bir dilim üzerinden hesaplanıyor — **RMS Period** slider'ıyla
1 ile 1500 ms arasında ayarlanıyor.

![FIFO halka tamponu](docs/fifo-diagram.png)

Pencere uzunluğu, algılayıcının "ne kadarını gördüğünü", dolayısıyla kompresörün
ne kadar agresif hissettirdiğini değiştiriyor. PluginDoctor ile ölçüldü; aynı
kaynak, aynı threshold, aynı ratio:

| RMS Period = 100 ms | RMS Period = 1000 ms |
|---|---|
| ![100 ms](docs/measure-rms-window-100ms.png) | ![1000 ms](docs/measure-rms-window-1000ms.png) |

Kısa pencere sinyali yakından takip ettiği için gain reduction çok değişkenlik
gösteriyor ve sıkıştırma agresif okunuyor. Uzun pencere materyalin genelini
ortalıyor, gain reduction daha sabit kalıyor ve etki daha yumuşak, seviye
dengeleyici bir karaktere bürünüyor.

### 2. Değiştirilebilir zarf eğrisi şekli

Bu, standart JUCE'de olmayan kısım.

Balistik filtre tek kutuplu (one-pole) bir yumuşatıcı. Her sample'da zarfı,
`cte` katsayısı kadar mevcut seviyeye doğru hareket ettiriyor:

```cpp
result = level + cte * (yold - level);
cte    = std::exp (expFactor / timeMs);
```

Standart JUCE'de `expFactor` koda gömülü:

```cpp
expFactor = -2.0 * pi * 1000.0 / sampleRate;   // -2.0 değiştirilemez
```

Burada attack ve release'in her biri kendi çarpanına sahip ve bu çarpan bir
parametre olarak dışarı açılmış:

```cpp
expFactorAttack  = attackCo  * pi * 1000.0 / sampleRate;   // -5.0 … -0.01
expFactorRelease = releaseCo * pi * 1000.0 / sampleRate;
```

Sonuç: **aynı attack süresine ayarlanmış iki kompresör, sinyali tamamen farklı
şekilde tutabiliyor.** Aşağıdaki iki ölçümde de Attack = 300 ms,
RMS Period = 100 ms. Sadece katsayı farklı:

| Attack Coefficient = −0.1 | Attack Coefficient = −5.0 |
|---|---|
| ![katsayı -0.1](docs/measure-attack-coef-0.1.png) | ![katsayı -5.0](docs/measure-attack-coef-5.0.png) |

−0.1'de zarf iki saniye sonra hâlâ oturmaya devam ediyor — gain reduction'a uzun
ve yavaş bir yaslanma. −5.0'da ise anında iniyor ve 300 ms içinde işini bitirmiş
oluyor. Diğer bütün ayarlar aynı.

Aynı kontrol release için de var:

| Release Coefficient = −1.0 | Release Coefficient = −5.0 |
|---|---|
| ![katsayı -1.0](docs/measure-release-coef-1.0.png) | ![katsayı -5.0](docs/measure-release-coef-5.0.png) |

Katsayı sıfıra yaklaştıkça cevap yavaşlıyor ve yumuşuyor; sıfırdan uzaklaştıkça
hızlanıyor ve sertleşiyor.

---

## Sinyal akışı

```
DAW buffer
   │
   ├──▶ Fifo.h ──────────▶ N sample çek (N = RMS Period × sample rate)
   │    (kilitsiz halka)        │
   │                            ▼
   │                    kanal başına getRMSLevel()
   │                            │
   │                   opsiyonel LinearSmoothedValue  ← "Enable smoothing"
   │                            │
   │                            ▼
   └──────────────────▶ juce::dsp::Compressor (yamalı)
                                │
                        BallisticsFilter (yamalı)
                        ├─ RMS modu  → zarf, pencerelenmiş RMS'i takip eder
                        └─ Peak modu → zarf, |sample| değerini takip eder
                                │
                    attack/release katsayıları eğriyi şekillendirir
                                │
                        zarf vs. threshold → gain reduction
                                │
                        × make-up gain ──────▶ çıkış
                                │
                     gain reduction (dB) ──▶ arayüz metresi, 24 Hz timer
```

Gain reduction metresi, analog iğneli bir gösterge olarak çizilmiş özel bir
`juce::Component` (`Source/GainReductionMeter.h`) — metafor bir araba hız
göstergesi: "daha hızlı", "daha çok gain reduction" demek.

---

## Parametreler

| Kontrol | Aralık | Varsayılan |
|---|---|---|
| Threshold | −60 … 0 dB | −20 dB |
| Ratio | 1, 2, 3 … 10, 15, 20, 50, 100 | 5.0 |
| Attack | 0.5 … 300 ms | 15 ms |
| Release | 5 … 1000 ms | 50 ms |
| **Attack Coefficient** | −5.0 … −0.01 | −0.1 |
| **Release Coefficient** | −5.0 … −0.01 | −0.4 |
| Make Up | −20 … +20 dB | 0 dB |
| **RMS Period** | 1 … 1500 ms | 50 ms |
| Algılama | RMS / Peak | RMS |
| Enable Smoothing | açık / kapalı | kapalı |
| Bypass | açık / kapalı | kapalı |

Bütün parametreler otomasyona uygun. Üst kısımda ayrıca kanal başına **Max RMS**
(yaklaşık son 4 saniyedeki en yüksek RMS) ve **Current RMS** gösteriliyor,
saniyede 24 kez yenileniyor.

**Peak** seçiliyken RMS Period'un etkisi yok — algılayıcı doğrudan sample'ın
genliğini okuyor.

---

## Derleme

macOS ve Xcode gerekiyor. Proje **JUCE 7.0.12** hedefliyor ve yamalı modüller
`modules/` içinde depoda bulunuyor — DSP'yi derlemek için ayrıca JUCE kurmana
gerek yok.

Tek pürüz şu: `JuceLibraryCode/` bir **JUCE 8** Projucer'ıyla üretilmiş, ama
`modules/` JUCE 7. Bu yüzden JUCE 8'e ait iki derleme birimi hariç tutulmalı.
Aşağıdaki komut çalıştığı doğrulanmış durumda (AU hedefi, `BUILD SUCCEEDED`,
`auval` geçiyor):

```bash
cd Builds/MacOSX
xcodebuild -project RMSCompressor.xcodeproj \
  -target "RMSCompressor - AU" -configuration Release \
  EXCLUDED_SOURCE_FILE_NAMES="include_juce_graphics_Harfbuzz.cpp include_juce_core_CompilationTime.cpp" \
  HEADER_SEARCH_PATHS="\$(SRCROOT)/../../JuceLibraryCode \$(SRCROOT)/../../modules \$(SRCROOT)/../../modules/juce_audio_plugin_client/AU \$(SRCROOT)/../../modules/juce_audio_processors/format_types/VST3_SDK" \
  build
```

VST3 için hedefi `"RMSCompressor - VST3"` olarak değiştir.

Xcode'un eklenti kopyalama adımı, her Release derlemesinde çıktıyı
`~/Library/Audio/Plug-Ins/Components/` altına kuruyor. Yeni sürümü görmesi için
DAW'ı kapatıp açman gerekiyor.

> Projeyi bir JUCE 7 Projucer'ıyla yeniden üretmek ya da yamaları JUCE 8'e
> taşımak, yukarıdaki bayraklara olan ihtiyacı ortadan kaldırır. Bkz.
> [Bilinen eksikler](#bilinen-eksikler).

---

## Depo yapısı

| Yol | Ne olduğu |
|---|---|
| `Source/PluginProcessor.*` | Parametreler, RMS hesabı, sinyal akışı |
| `Source/PluginEditor.*` | Arayüz, 24 Hz yenileme timer'ı |
| `Source/Fifo.h` | Hesaplama penceresi için kilitsiz halka tamponu |
| `Source/GainReductionMeter.h` | Analog tarzı özel iğneli metre |
| `modules/juce_dsp/widgets/juce_Compressor.*` | **Yamalı** — RMS algılama yolu, make-up gain, bypass |
| `modules/juce_dsp/processors/juce_BallisticsFilter.*` | **Yamalı** — zarf katsayıları dışarı açıldı |
| `docs/` | Bu README'de kullanılan ekran görüntüleri ve ölçümler |

### Dallar

| Dal | Amacı |
|---|---|
| `au-release` | **Varsayılan.** Çalışan hat — yamalı modüller, derleniyor ve `auval` geçiyor |
| `main` | Arşiv. Üçüncü bir algılama modu eklemeye çalışan, yarım kalmış bir deneme; DSP tarafı kaybolduğu için derlenmiyor |
| `arsiv-2024-nisan` | Arşiv. JUCE DSP modülleri yamalanmadan önceki, Nisan 2024 tarihli erken bir anlık görüntü |

---

## Bilinen eksikler

Dürüst liste. Bu kod C++ öğrenilirken yazıldı ve Ağustos 2026'da yapılan bir
inceleme düzeltilmeye değer şeyler ortaya çıkardı. İşaretli maddeler
düzeltilmiş olanlar; kalanlar açık.

- [ ] **Audio thread'de her sample için heap tahsisi.** `processSample`,
      `std::vector<float> rmsLevels`'ı sample döngüsünün içinde değer olarak
      alıyor — kanal başına saniyede yaklaşık 44.100 tahsis. Çözüm `const&`
      ile geçirmek.
- [ ] **Ratio, değeri yerine indeksiyle başlatılıyor.** `prepareToPlay`, seçim
      listesinin indeksini doğrudan `setRatio()`'ya veriyor. Ratio 1.0 ile
      kaydedilmiş bir oturum açıldığında `setRatio(0)` çağrılıyor ve sıfıra
      bölme oluyor.
- [ ] **Mono'da sınır dışı okuma.** Arayüz koşulsuz olarak 1 numaralı kanalı
      okuyor; `isBusesLayoutSupported` ise mono'ya izin veriyor.
- [ ] **`prepareToPlay` bütün parametreleri aktarmıyor.** Attack, release,
      make-up gain ve bypass yalnızca bir sonraki parametre değişiminde DSP'ye
      ulaşıyor; oturum yüklendikten sonraki ilk bloklar eski değerlerle
      işlenebiliyor.
- [ ] **Gain reduction değerinde veri yarışı.** Düz bir `float`, audio
      thread'den yazılıp arayüz thread'inden okunuyor; RMS tamponuna da iki
      yerden yazılıyor, bu da FIFO'nun tek-yazar sözleşmesini bozuyor.
- [ ] **Balistik filtrede varsayılan dönüş değeri yok.** Ne peak ne rms seçili
      olduğunda `processSample` değer döndürmeden fonksiyonun sonuna varıyor.
- [ ] **Gain reduction metresi kalibre değil.** Ölçek işaretleri eşit açı
      aralıklarıyla çiziliyor ama temsil ettikleri değerler doğrusal değil;
      dolayısıyla iğne, uçlar dışında etiketlerle uyuşmuyor.
- [ ] **`modules/` ile `JuceLibraryCode/` arasında JUCE 7 / JUCE 8
      uyumsuzluğu**; yukarıdaki derleme bayraklarını gerektiriyor.
- [ ] `processBlock` içinde isim gölgeleme; `resized()` içinden `setSize()`
      çağrısı.

---

## Proje bağlamı

**İstanbul Teknik Üniversitesi** Müzik Teknolojileri bölümünde, 2024 bahar
döneminde **Serbest Proje Çalışması 2** dersi için geliştirildi. Teslim tarihi
7 Haziran 2024. Danışman: **Dr. Ozan Sarıer** — değiştirilebilir pencere fikri
de kendisinin önerisiyle ortaya çıktı.

Döneme neredeyse hiç programlama bilgisi olmadan başladım — biraz JavaScript,
C++ ise hiç yoktu. Kompresör ve dilin kendisi tek bir dönem içinde paralel
olarak öğrenildi.

Projenin tam raporu (29 sayfa, Türkçe) DSP arka planını, geliştirme sürecini,
yol boyunca karşılaşılan problemleri ve PluginDoctor ölçümlerini kapsıyor. Bu
depoda yer almıyor; isteyene iletilebilir.

### Teşekkür

**Akash Murthy** — hiç tanımadığı birinden gelen bir e-postanın ardından bir
saatini ayırıp Zoom görüşmesi yaptı ve FIFO halka tamponunun, RMS algılamayı
host buffer boyutundan nasıl bağımsızlaştırabileceğini anlattı. O görüşme,
projenin en kritik problemini çözdü.

**Dr. Ozan Sarıer** — projenin fikri ve danışmanlığı için.

---

## Lisans

Bu proje, JUCE 7 modüllerinin değiştirilmiş kopyalarını içeriyor ve bunlar
JUCE'un GPLv3 seçeneği kapsamında kullanılıyor (JUCE splash screen açık).
Dolayısıyla her türlü dağıtım **GPLv3** kapsamına giriyor.
