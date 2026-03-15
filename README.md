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

**Önemli alanlar:**
| Alan | Açıklama | Örnek |
|------|----------|-------|
| `id` | Benzersiz kimlik | `com.ahmet.harita-eklentisi` |
| `name` | Görünen isim | `Harita Eklentisi` |
| `version` | Semantik versiyon | `0.1.0` |
| `entry` | DLL dosya adı | `harita-eklentisi.dll` |
| `apiVersion` | Plugin API sürümü (şimdilik `1`) | `1` |

### 3. Kodunuzu Yazın

`plugin.cpp` dosyasını açın. İçinde 5 zorunlu fonksiyon var — bunlar eklentinizin Launcher ile iletişim kurmasını sağlar:

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

Bu fonksiyonların hepsini implement etmeniz **zorunludur**. Kendi mantığınızı `makineai_initialize` ve `makineai_shutdown` içine yazın.

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
- manifest.json'u doğrular
- Yasak dosya türlerini kontrol eder
- zstd ile sıkıştırır (seviye 22, maksimum sıkıştırma)
- `.makine` dosyası oluşturur
- SHA-256 sağlama toplamını gösterir

### 6. Yayınlayın

1. GitHub'da bir **Release** oluşturun (örn: `v0.1.0`)
2. `.makine` dosyasını release'e **asset olarak** ekleyin
3. Repo ayarlarında **Topics** kısmına `makineai-plugin` ekleyin

Bu kadar! Eklentiniz artık MakineAI Launcher'ın **Eklentiler** sayfasında görünecek.

---

## Proje Yapısı

```
├── manifest.json              — Eklenti bilgileri (isim, versiyon, DLL adı)
├── plugin.cpp                 — Ana kaynak dosya (5 zorunlu fonksiyon)
├── CMakeLists.txt             — Derleme yapılandırması
└── include/makineai/plugin/   — SDK başlık dosyaları (dokunmayın)
    ├── plugin_api.h           — Fonksiyon tipleri
    └── plugin_types.h         — Hata kodları ve yapılar
```

## Hata Kodları

Eklentiniz şu hata kodlarını döndürebilir:

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

**makine-pack.py aracını nereden bulurum?**
- [MakineAI-Launcher/scripts/tools/makine-pack.py](https://github.com/MakineCeviri/MakineAI-Launcher/blob/main/scripts/tools/makine-pack.py)

## Yardım ve İletişim

- [MakineAI-Launcher Issues](https://github.com/MakineCeviri/MakineAI-Launcher/issues) — Hata bildirimi
- [MakineAI-Plugins](https://github.com/MakineCeviri/MakineAI-Plugins) — Eklenti kayıt defteri

## Lisans

GPL-3.0
