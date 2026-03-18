# MakineAI Eklenti Geliştirme Şablonu

MakineAI Launcher için eklenti geliştirmek isteyenlere hazır başlangıç noktası.

GitHub'da **"Use this template"** butonuna tıklayarak kendi eklentinizi bu şablondan oluşturabilirsiniz.

---

## Hızlı Başlangıç

### 1. Şablondan Repo Oluşturun

GitHub'da bu sayfanın sağ üstündeki yeşil **"Use this template"** butonuna tıklayın. Kendi hesabınızda yeni bir repo oluşturulacak.

### 2. manifest.json Dosyasını Düzenleyin

```json
{
    "id": "com.sizin-kullanici-adiniz.eklenti-adi",
    "name": "Eklentinizin Adı",
    "version": "0.1.0",
    "entry": "eklenti-adi.dll",
    ...
}
```

**Zorunlu alanlar:**

| Alan | Açıklama | Örnek |
|------|----------|-------|
| `id` | Benzersiz kimlik (reverse domain) | `com.ahmet.harita-eklentisi` |
| `name` | Görünen isim | `Harita Eklentisi` |
| `version` | Semantik versiyon | `0.1.0` |
| `entry` | DLL dosya adı | `harita-eklentisi.dll` |
| `apiVersion` | Plugin API sürümü (şimdilik `1`) | `1` |

**Opsiyonel alanlar:**

| Alan | Açıklama | Örnek |
|------|----------|-------|
| `category` | Eklenti kategorisi (aşağıya bakın) | `"translation"` |
| `description` | Kısa açıklama | `"Harita üzerinde çeviri"` |
| `author` | Geliştirici adı | `"Ahmet Yılmaz"` |
| `license` | Lisans | `"GPL-3.0"` |
| `homepage` | Proje sayfası | `"https://github.com/..."` |
| `capabilities` | Özel yetenekler listesi | `["ocr", "overlay"]` |
| `features` | Kullanıcıya gösterilecek özellikler | `["OCR Desteği"]` |
| `platforms` | Desteklenen platformlar | `["win64"]` |
| `minLauncherVersion` | Gereken minimum Launcher sürümü | `"0.1.0"` |
| `settings` | Kullanıcı ayarları tanımları | Aşağıya bakın |

**Kategoriler:**

| Değer | Açıklama |
|-------|----------|
| `translation` | Çeviri motorları |
| `accessibility` | Erişilebilirlik (OCR, overlay vb.) |
| `hook` | Oyun yakalama (hook) |
| `other` | Diğer |

### 3. Kodunuzu Yazın

`plugin.cpp` dosyasını açın. İçinde **5 zorunlu** ve **4 opsiyonel** fonksiyon var.

#### Zorunlu Fonksiyonlar (5)

Bunlar eklentinizin Launcher ile iletişim kurmasını sağlar — **hepsini implement etmeniz zorunludur:**

```cpp
// Eklenti bilgilerini döndürür
extern "C" MakineAiPluginInfo makineai_get_info(void);

// Eklentiyi başlatır (dataPath: verilerinizi saklayacağınız dizin)
extern "C" MakineAiError makineai_initialize(const char* dataPath);

// Eklentiyi kapatır, kaynakları serbest bırakır
extern "C" void makineai_shutdown(void);

// Eklentinin hazır olup olmadığını döndürür
extern "C" bool makineai_is_ready(void);

// Son hata mesajını döndürür
extern "C" const char* makineai_get_last_error(void);
```

#### Opsiyonel: Ayar Fonksiyonları (2)

Manifest'inizde `"settings"` dizisi tanımlıyorsanız, bu fonksiyonları da implement edin. Launcher kullanıcı arayüzünden ayar okuma/yazma yapabilir:

```cpp
// Ayar değerini döndürür (key = manifest settings[].key)
extern "C" const char* makineai_get_setting(const char* key);

// Ayar değerini günceller (Launcher UI'dan çağrılır)
extern "C" void makineai_set_setting(const char* key, const char* value);
```

#### Opsiyonel: OCR + Çeviri Fonksiyonları (2)

Erişilebilirlik (accessibility) eklentileri için — ekran yakalama, OCR ve çeviri:

```cpp
// Belirtilen ekran bölgesini yakala, OCR uygula, çevir, sonucu döndür
extern "C" const char* makineai_capture_ocr_translate(void* hwnd, int x, int y, int w, int h);

// Son yakalamadaki ham OCR metnini döndür (çeviri öncesi)
extern "C" const char* makineai_get_last_ocr_text(void);
```

> **Not:** `plugin.cpp` dosyasında opsiyonel fonksiyonlar yorum bloğu içinde hazır bulunmaktadır. İhtiyacınız olanların yorumunu kaldırın.

### 4. Derleyin

**Gereksinimler:**
- CMake 3.25 veya üstü
- MinGW GCC 13.1+ (veya herhangi bir C++23 derleyici)
- Ninja (opsiyonel ama önerilen)

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

Başarılı derleme sonrası `build/release/` dizininde DLL ve manifest.json dosyanız oluşur.

### 5. Paketleyin

`.makine` formatında paketlemek için:

```bash
pip install zstandard
python makine-pack.py ./build/release/
```

Bu komut şunları yapar:
- manifest.json'u doğrular (zorunlu alanlar: `id`, `name`, `version`, `apiVersion`, `entry`)
- Yasak dosya türlerini kontrol eder (`.exe`, `.bat`, `.cmd`, `.ps1`, `.vbs`, `.msi`, `.js`)
- zstd ile sıkıştırır (seviye 22, maksimum sıkıştırma)
- `.makine` dosyası oluşturur (MKPK v2 formatı)
- SHA-256 sağlama toplamını gösterir

**Opsiyonlar:**

```bash
python makine-pack.py ./build/release/ --output my-plugin-1.0.0.makine
python makine-pack.py ./build/release/ --level 19  # daha hızlı, biraz daha büyük
```

### 6. Yayınlayın

1. GitHub'da bir **Release** oluşturun (örn: `v0.1.0`)
2. `.makine` dosyasını release'e **asset olarak** ekleyin
3. Repo ayarlarında **Topics** kısmına `makineai-plugin` ekleyin

Bu kadar! Eklentiniz artık MakineAI Launcher'ın **Eklentiler** sayfasında görünecek.

---

## Proje Yapısı

```
├── manifest.json              — Eklenti bilgileri (isim, versiyon, DLL adı, ayarlar)
├── plugin.cpp                 — Ana kaynak dosya (5 zorunlu + 4 opsiyonel fonksiyon)
├── CMakeLists.txt             — Derleme yapılandırması
├── makine-pack.py             — .makine paketleme aracı
└── include/makineai/plugin/   — SDK başlık dosyaları (dokunmayın)
    ├── plugin_api.h           — Fonksiyon tipleri (typedef'ler)
    └── plugin_types.h         — Hata kodları, yapılar ve kategoriler
```

## manifest.json — Ayarlar (Settings)

Eklentinizin kullanıcı tarafından değiştirilebilir ayarları varsa, `"settings"` dizisine tanımlayın. Launcher bu bilgiyi okuyarak otomatik bir ayar arayüzü oluşturur:

```json
{
    "settings": [
        {
            "key": "language",
            "label": "Hedef Dil",
            "type": "select",
            "default": "tr",
            "options": ["tr", "en", "de", "fr"],
            "description": "OCR sonuçlarının çevrileceği dil"
        },
        {
            "key": "notifications",
            "label": "Bildirimler",
            "type": "toggle",
            "default": "true",
            "description": "Çeviri tamamlandığında bildirim göster"
        }
    ]
}
```

**Setting alanları:**

| Alan | Zorunlu | Açıklama |
|------|---------|----------|
| `key` | Evet | Benzersiz ayar anahtarı |
| `label` | Evet | Kullanıcıya gösterilecek etiket |
| `type` | Evet | `"select"` veya `"toggle"` |
| `default` | Evet | Varsayılan değer |
| `options` | `select` için | Seçenekler listesi |
| `description` | Hayır | Açıklama metni |

> **Önemli:** `"settings"` tanımladıysanız, `plugin.cpp`'de `makineai_get_setting` ve `makineai_set_setting` fonksiyonlarını da implement edin. Aksi halde Launcher ayar arayüzünü gösterir ama değerler okunamaz/yazılamaz.

## Hata Kodları

Eklentiniz şu hata kodlarını döndürebilir (`MakineAiError` enum):

| Kod | Anlamı |
|-----|--------|
| `MAKINEAI_OK` | Başarılı |
| `MAKINEAI_ERR_INIT_FAILED` | Başlatma başarısız |
| `MAKINEAI_ERR_NOT_READY` | Henüz hazır değil |
| `MAKINEAI_ERR_INVALID_PARAM` | Geçersiz parametre |
| `MAKINEAI_ERR_NOT_FOUND` | Bulunamadı |
| `MAKINEAI_ERR_ACCESS_DENIED` | Erişim engellendi |
| `MAKINEAI_ERR_UNSUPPORTED` | Desteklenmiyor |
| `MAKINEAI_ERR_TIMEOUT` | Zaman aşımı |
| `MAKINEAI_ERR_ENGINE_ERROR` | Motor hatası |

## Sık Sorulan Sorular

**Eklentim Launcher'da neden görünmüyor?**
- Repo'nuza `makineai-plugin` topic'i eklediğinizden emin olun
- GitHub API topic aramasının güncellenmesi birkaç dakika sürebilir

**Hangi derleyiciyi kullanmalıyım?**
- MinGW GCC 13.1+ önerilir (MakineAI Launcher bununla derlenir)
- MSVC ile de derlenebilir ancak ABI uyumluluğu C ABI ile sağlanır

**Eklentim kullanıcı verisi nereye kaydeder?**
- `makineai_initialize(dataPath)` ile verilen dizine: `AppData/Local/MakineAI/plugin-data/<eklenti-id>/`

**Opsiyonel fonksiyonları implement etmek zorunda mıyım?**
- Hayır. Sadece 5 zorunlu fonksiyon yeterlidir. Ayar ve OCR fonksiyonları ihtiyacınız varsa ekleyin.

**Settings tanımladım ama Launcher'da değerler değişmiyor?**
- `makineai_get_setting` ve `makineai_set_setting` fonksiyonlarını DLL'de export ettiğinizden emin olun.

## Yardım ve İletişim

- [MakineAI-Launcher Issues](https://github.com/MakineCeviri/MakineAI-Launcher/issues) — Hata bildirimi
- [MakineAI-Plugins](https://github.com/MakineCeviri/MakineAI-Plugins) — Eklenti kayıt defteri

## Lisans

GPL-3.0
