# Makine Eklenti Şablonu

[Makine Launcher](https://github.com/MakineCeviri/Makine-Launcher) için eklenti (plugin ABI 2) başlangıç deposu.

Bir Makine eklentisi, 64 bit bir Windows DLL'i ile bir `manifest.json` dosyasından oluşur ve `.mkplugin` paketi olarak dağıtılır. Makine eklenti kodunu kendi içine yüklemez: her etkin eklenti ayrı bir `makine-plugin-host.exe` sürecinde çalışır, böylece çöken ya da donan bir eklenti yalnızca kendini durdurur.

Manifest alanlarının, host API'sinin ve olayların tamamı **[geliştirici belgelerinde](https://makineceviri.org/docs/plugin-api)** (Türkçe) ve **[eklenti rehberinde](https://github.com/MakineCeviri/Makine-Launcher/blob/dev/docs/plugins/README.md)** (İngilizce). Bu dosya yalnızca bir özet.

---

## Hızlı başlangıç

1. **Şablonu kullanın.** GitHub'da **Use this template** ile kendi deponuzu oluşturun.
2. **`manifest.json` dosyasını düzenleyin.** En azından `id` (ters DNS, küçük harf: `com.adiniz.eklenti`), `name`, `publisher` ve `homepage` alanlarını değiştirin. DLL'in adı ve paketteki yeri `entry` alanından gelir (`bin/my-plugin.dll`); CMake bu alanı okur.
3. **Derleyin.** Gerekenler: CMake 3.25+, MinGW-w64 GCC 13+ ya da Visual Studio 2022+ (MSVC). İlk yapılandırmada CMake, JSON kütüphanesini ([nlohmann/json](https://github.com/nlohmann/json) 3.12.0, sabit sürüm ve SHA-256 doğrulamalı) indirir; bunun için internet bağlantısı gerekir.

   ```bash
   cmake -B build -G Ninja      # MSVC için: cmake -B build -A x64
   cmake --build build          # MSVC için: cmake --build build --config Release
   ```

   Sonuç `build/package/` klasöründedir; Makine'nin kurduğu eklentinin aynısı:

   ```
   build/package/
   ├── manifest.json
   └── bin/my-plugin.dll
   ```

4. **Makine olmadan çalıştırın.** `tools/plugin_dev.py`, eklentiyi gerçek `makine-plugin-host.exe` içinde çalıştırır, launcher'ın yerine geçerek host çağrılarını yanıtlar ve eklentinin söylediği her şeyi yazdırır:

   ```bash
   python tools/plugin_dev.py build/package --host "C:\Makine\makine-plugin-host.exe" --invoke hello

   export MAKINE_PLUGIN_HOST="C:\Makine\makine-plugin-host.exe"   # her seferinde --host yazmamak için
   python tools/plugin_dev.py build/package --invoke hello \
       --event settings.changed '{"key":"greeting","value":"Selam!"}' --invoke hello
   ```

   `--invoke` ve `--event` komut satırındaki sırayla çalışır: ikinci örnek önce selam verir, sonra ayarı değiştirir, sonra yeni metinle bir kez daha selam verir. İkisini de vermezseniz komutları stdin'den okur: `hello`, `!settings.changed {"key":"showUptime","value":false}`, `quit`. Başlangıç ayarlarını `--set interval=5` ile değiştirebilirsiniz.

   Host'u `--host` ya da `MAKINE_PLUGIN_HOST` ile göstermek zorunludur. Eklentinin `dataDir` klasörü bu modda `build/package/.dev-data/` olur (pakete girmez).

5. **Paketleyin.**

   ```bash
   pip install zstandard
   cmake --build build --target pack     # ya da: python tools/plugin_registry.py pack build/package
   ```

   Çıktı: `build/<id>-<version>.mkplugin`. Paketleyici manifest'i denetler, betik ve yükleyici dosyalarını (`.bat`, `.ps1`, `.msi`, `.js`…) reddeder.

6. **Makine'de deneyin.** Ayarlar → Eklentiler → **Geliştirici modu**'nu açın → **Dosyadan yükle** ile `.mkplugin` dosyasını seçin. Kayıt defterinden gelmeyen eklentiler *Doğrulanmamış* olarak işaretlenir.

Her push ve pull request'te `.github/workflows/build.yml` eklentiyi MSVC ile derler, paketler ve `.mkplugin` dosyasını iş akışı çıktısı (artifact) olarak yükler.

## `makine-plugin-host.exe` nerede?

Makine'nin taşınabilir ZIP'inde (0.2.0 ve sonrası) `Makine-Launcher.exe`'nin yanında gelir. ZIP'i bir klasöre açıp `--host` ile o dosyayı gösterin. Makine-Launcher'ı kaynaktan derliyorsanız `just dev` sonrası `build/dev/makine-plugin-host.exe` konumundadır.

## Depo yapısı

```
manifest.json               kimlik, sürüm, izinler, ayarlar, eylemler, olaylar
plugin.cpp                  örnek eklenti (sdk.hpp üzerine)
CMakeLists.txt              nlohmann/json'u indirir, DLL'i statik C++ çalışma zamanıyla derler, build/package/ düzenini kurar, pack hedefi
include/makine/plugin/      abi.h + sdk.hpp: Makine-Launcher'dan kopya, düzenlemeyin
tools/plugin_dev.py         eklentiyi launcher olmadan gerçek host'ta çalıştırır
tools/plugin_registry.py    manifest denetimi ve .mkplugin paketleme
.github/workflows/build.yml derleme + paketleme (CI)
```

`include/` ve `tools/` altındaki dosyalar Makine-Launcher deposundan kopyadır (`core/include/makine/plugin/`, `scripts/`); her dosyanın başında kaynağı yazar. Güncellemek için yeni sürümü üzerine kopyalayıp bu ilk satırı koruyun.

## Örnek eklenti ne gösteriyor?

`plugin.cpp`, çoğu eklentinin ihtiyaç duyduğu parçaları tek dosyada gösterir:

- **Ayarlar:** `initialize` içinde init JSON'dan okunur, `settings.changed` olayıyla güncel tutulur.
- **Eylem:** eklenti sayfasındaki *Selam ver* düğmesi `invoke("hello")` çağırır; eklenti `ui.notify` ile bir bildirim gösterir.
- **Durum satırı:** `initialize` içinde başlatılan, `shutdown` içinde durdurulup beklenen (join) bir işçi thread'i `ui.setStatus` ile eklenti adının altındaki satırı günceller.
- **Dosya:** `dataDir` altına `hello.log` yazar; UTF-8 yol `MultiByteToWideChar` ile UTF-16'ya çevrilip geniş (wide) Win32 API'siyle açılır.

JSON için [nlohmann/json](https://github.com/nlohmann/json) kullanılır. ABI yalnızca metin gördüğü için başka bir kütüphaneyle de değiştirebilirsiniz. Ayrıştırma hatası gibi bir exception `initialize`, `invoke` ya da `onEvent` içinde fırlatılırsa `sdk.hpp` onu yakalar. Kendi thread'inizde yakalanmayan bir exception ise süreci sonlandırır.

## Uyulması gereken kurallar

| Kural | Neden |
|-------|-------|
| `initialize`, `invoke`, `onEvent`, `shutdown` tek bir host thread'inde, sırayla çalışır | Aralarında kilit gerekmez. Uzun ya da sürekli iş kendi thread'inizde yapılır. |
| `host()` fonksiyonları her thread'den çağrılabilir; `call` Makine yanıt verene kadar bekler | Kullanıcıyı bekleyen çağrıları (`overlay.pickRegion`, `games.requestAttach`) işçi thread'inden yapın. |
| `DllMain` ya da giriş fonksiyonunda iş yapmayın | `initialize` içinde başlayın; thread'leri `shutdown` içinde durdurun (3 sn süre, sonra süreç sonlandırılır). |
| `stdout` hiçbir yere gitmez; `stderr` satırları eklenti günlüğünde görünür | `stdout` protokol kanalıdır. |
| JSON'daki yollar UTF-8'dir | `MultiByteToWideChar(CP_UTF8, …)` + geniş API kullanın; dar `fopen` Türkçe kullanıcı adlarında (`Şükrü`) bozulur. |
| DLL'ler yalnızca eklenti klasöründe, launcher klasöründe ve System32'de aranır | C++ çalışma zamanını statik bağlayın (bu şablonun CMake'i öyle yapar) ya da gereken DLL'leri eklenti klasörüne koyun. |
| Exception ABI sınırını geçemez | `sdk.hpp` yakalar ve `MAKINE_E_FAILED` döndürür. |

Süreler: `initialize` 15 sn, `invoke` 30 sn (eylemler 120 sn). Beş dakika içinde üç çökme, eklentiyi kullanıcı yeniden başlatana kadar durdurur.

## Manifest (özet)

| Alan | Kural |
|------|-------|
| `schema`, `abi` | `2` |
| `id` | ters DNS, küçük harf (`com.adiniz.eklenti`); kurulum klasörünün adı da budur |
| `version`, `minLauncher` | semver (`1.2.0`, `0.2.0-beta`) |
| `entry` | DLL'in paket içindeki yolu, `..` olmadan |
| `publisher` | zorunlu |
| `capabilities` | her biri `{id, reason}`; Türkçe `reason` kurulumda kullanıcıya gösterilir. Kimlikler: `network`, `screen-capture`, `overlay`, `game-files-read`, `game-files-write`, `process-inject`. Yalnızca gerçekten gerekenleri isteyin; kullanıcı onaylamadan eklenti çalışmaz. |
| `contributes.settings` | Makine'nin çizdiği ayarlar: `toggle`, `select`, `text`, `secret`, `number`, `path` |
| `contributes.actions` | eklenti sayfasındaki düğmeler; basılınca `invoke(id)` çağrılır |
| `contributes.events` | eklentinin alacağı olaylar: `game.started`, `game.stopped`, `translation.installed`, `translation.uninstalled`, `settings.changed` |

## Host API (özet)

`host().call(method, paramsJson)`: `settings.get` · `settings.all` · `settings.set` · `ui.notify` · `ui.setStatus` · `games.running` · `games.list` · `games.requestAttach` · `overlay.pickRegion` · `overlay.show` · `overlay.hide`. Bazıları bir izin ister (`games.list` → `game-files-read`, `overlay.*` → `overlay`, `games.requestAttach` → `process-inject`). Parametreler ve dönüş değerleri rehberde.

## Yayınlama

Makine, eklenti listesini imzalı kayıt defterinden alır; imzası ya da paket SHA-256'sı tutmayan hiçbir şeyi kurmaz. Eklentinizin listede yer alması için:

1. Kaynak kodunuzu herkese açık bir depoda tutun ve bir sürüm etiketi (`v0.1.0`) oluşturun.
2. [MakineCeviri/Makine-LauncherPlugins](https://github.com/MakineCeviri/Makine-LauncherPlugins) deposuna depo adresinizi ve sürüm etiketini içeren bir pull request açın.
3. Makine ekibi kodu inceler, eklentiyi kendisi derleyip paketler, kayıt defterine ekler ve imzalar.

Kayıt defterinde olmayan eklentiler yalnızca geliştirici modunda, dosyadan kurulabilir.

## Yardım

- [Geliştirici belgeleri](https://makineceviri.org/docs/plugin-api)
- [Eklenti rehberi](https://github.com/MakineCeviri/Makine-Launcher/blob/dev/docs/plugins/README.md)
- [Makine-Launcher Issues](https://github.com/MakineCeviri/Makine-Launcher/issues)

## Lisans

[LICENSE](LICENSE) dosyasına bakın.
