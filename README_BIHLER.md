# LLVM clang-tidy mit Bihler-Modul

Diese Datei beschreibt den aktuell funktionierenden Stand für:
- Build von `clang-tidy` mit Bihler-Modul
- Integration eigener Checks
- Update auf neue upstream Release-Tags

Die Anleitung ist auf den aktuellen Workspace ausgelegt:
- Repository: `C:\TFS\Spielwiese\ClangTidyBihler\llvm-project`
- Build-Verzeichnis: `C:\TFS\Spielwiese\ClangTidyBihler\llvm-build`
- Build-Skript: `llvm-project\build_bihler_module.bat`

---

## 1. Voraussetzungen

- Visual Studio 2022 mit C++-Workload
- Git
- Python 3.8+
- Genug Speicherplatz und RAM für LLVM-Builds

Empfohlene VS-Komponenten:
- Desktop development with C++
- C++ CMake tools for Windows
- Windows SDK
- MSVC Build Tools (x64/x86)

---

## 2. Repository-Setup (Fork + upstream)

Im Fork-Repository:

```powershell
git remote -v
```

Erwartet:
- `origin` = eigener Fork
- `upstream` = `https://github.com/llvm/llvm-project.git`

Tags und Branches aktualisieren:

```powershell
git fetch upstream --tags --prune
git fetch origin --prune
```

---

## 3. Build mit dem aktuellen Bihler-Skript

Das Skript `build_bihler_module.bat` ist die Quelle der Wahrheit für euren Build-Flow.

Es macht aktuell:
- VS-Umgebung über `VsDevCmd.bat`
- Ninja aus VS-CMake-Installation in `PATH`
- CMake-Konfiguration mit
  - `-G Ninja`
  - `-DCMAKE_BUILD_TYPE=Release`
  - `-DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra"`
  - `-DLLVM_TARGETS_TO_BUILD="X86"`
  - `-DLLVM_ENABLE_ASSERTIONS=ON`
- Build von Target `clang-tidy`
- Prüfung auf `bin\clang-tidy.exe`
- Check-Listing auf `bihler`

Ausführen:

```powershell
cd C:\TFS\Spielwiese\ClangTidyBihler\llvm-project
.\build_bihler_module.bat
```

---

## 4. Bihler-Modul Integration (Kurzüberblick)

Relevante Dateien:
- `clang-tools-extra/clang-tidy/bihler/CMakeLists.txt`
- `clang-tools-extra/clang-tidy/bihler/BihlerTidyModule.cpp`
- `clang-tools-extra/clang-tidy/bihler/BihlerUnsafeAllocationCheck.h`
- `clang-tools-extra/clang-tidy/bihler/BihlerUnsafeAllocationCheck.cpp`

Zentrale Integration:
- `add_subdirectory(bihler)` in `clang-tools-extra/clang-tidy/CMakeLists.txt`
- `clangTidyBihlerModule` in `ALL_CLANG_TIDY_CHECKS`
- Bei aktivierten query-basierten checks zusätzlich:
  - `add_subdirectory(custom)` unter `if(CLANG_TIDY_ENABLE_QUERY_BASED_CUSTOM_CHECKS)`

Wichtig bei neueren LLVM-Versionen:
- Kein Include von `ClangTidyModuleRegistry.h` verwenden
- Stattdessen `ClangTidyModule.h` nutzen

---

## 5. Funktion prüfen

Nach erfolgreichem Build:

```powershell
cd C:\TFS\Spielwiese\ClangTidyBihler\llvm-build
.\bin\clang-tidy.exe --version
.\bin\clang-tidy.exe --list-checks --checks=* | findstr /i bihler
```

Erwartung: mindestens `bihler-unsafe-allocation` wird gelistet.

---

## 6. Update auf neuen Release-Tag

Beispiel: Update auf `llvmorg-22.2.0`.

1. Tags holen:

```powershell
git fetch upstream --tags --prune
git fetch origin --prune
```

2. Release-Branch vom Tag erstellen:

```powershell
git switch -c release/llvmorg-22.2.0-bihler llvmorg-22.2.0
```

3. Bihler-Commits übernehmen:

Einfache Standard-Variante mit Patch-Branch (`bihler-patches`):

```powershell
git cherry-pick llvmorg-22.1.0..bihler-patches
```

Alternative (falls kein Patch-Branch genutzt wird):

```powershell
git cherry-pick <first-bihler-commit>^..<last-bihler-commit>
```

4. Build und Smoke-Test:

```powershell
cd C:\TFS\Spielwiese\ClangTidyBihler\llvm-project
.\build_bihler_module.bat
```

5. Branch veröffentlichen:

```powershell
git push -u origin release/llvmorg-22.2.0-bihler
```

Warum dieser Weg stabil ist:
- Ein upstream Release-Tag enthält nicht automatisch eure Fork-Änderungen
- Darum Release immer vom Tag starten und eure Patches gezielt übernehmen

---

## 7. Empfohlener Dauerprozess

- `main`: Integrations-/Historien-Branch im Fork
- `release/llvmorg-<version>-bihler`: pro Release ein sauberer Arbeitsbranch
- Alte temporäre Backport-Branches nach Merge löschen

Einfache Praxis:
- `bihler-patches`: langlebiger Branch nur mit Bihler-Änderungen
- Pro neuem Release: `git cherry-pick <previous-release-tag>..bihler-patches`

---

## 8. Häufige Probleme

### `LNK1181: clangTidyCustomModule.lib`
Ursache: `clangTidyCustomModule` wird gelinkt, aber `custom` nicht gebaut.
Fix: `add_subdirectory(custom)` im passenden `if(CLANG_TIDY_ENABLE_QUERY_BASED_CUSTOM_CHECKS)` Block.

### `fatal error C1021` mit `ClangTidyModuleRegistry.h`
Ursache: veralteter Header mit `#warning`.
Fix: auf `ClangTidyModule.h` umstellen.

### Meldung `Der Befehl "R" ist entweder falsch geschrieben ...`
Ursache: meist lokales `cmd`-AutoRun Umgebungsproblem.
Hinweis: kein LLVM-Quellcodeproblem, aber kann Exitcode/Lauf stören.

---

## 9. Lizenz / Upstream-Nähe

Custom-Code möglichst isoliert unter `clang-tools-extra/clang-tidy/bihler` halten.
So bleiben Updates auf neue LLVM-Releases planbar und konfliktarm.
