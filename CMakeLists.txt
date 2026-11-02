# Studio Bol Vocal Manager — cloud build via GitHub Actions

Dit project is broncode voor een experimentele JUCE VST3. Het is **nog geen geteste of volledig functionele plugin**. De cloudbuild kan de huidige broncode op een Windows-runner proberen te compileren; een geslaagde compile bewijst nog niet dat de DSP goed klinkt of dat Cubase 13 compatibiliteit in de praktijk is gevalideerd.

## Browser-only route (geen Visual Studio of JUCE op de eigen pc)

1. Maak via github.com een nieuw leeg repository aan, bijvoorbeeld `studio-bol-vocal-manager`. Kies bij het aanmaken geen README, .gitignore of licentie.
2. Pak dit ZIP-bestand uit op je computer.
3. Open het nieuwe repository op GitHub en kies **Add file → Upload files**.
4. Sleep de uitgepakte projectbestanden en mappen (inclusief `.github`) naar de uploadpagina. Let erop dat `.github/workflows/build-windows.yml` mee wordt geüpload.
5. Commit de bestanden naar `main`.
6. Open het tabblad **Actions**. De workflow **Build Studio Bol Vocal Manager (Windows VST3)** start automatisch.
7. Als de workflow groen eindigt, open je de run en download je onder **Artifacts** het pakket `Studio-Bol-Vocal-Manager-Windows-x64-VST3`.

GitHub Actions gebruikt een Windows Server 2022-runner, haalt JUCE 8.0.6 op via CMake FetchContent en probeert de VST3 in x64 Release te bouwen. GitHub-account en internettoegang zijn vereist. Ik kan niet namens jou inloggen, het repository aanmaken of de workflow starten.

## Huidige functionele beperkingen

- De huidige analyzer meet de volledige referentieaudio; er is **geen** AI/neural vocal source separation ingebouwd. Bij een volledig gemixt nummer beïnvloeden drums, bas en instrumenten dus de analyse. Voor een zinvolle match is voorlopig een geïsoleerde vocal stem nodig.
- De plugin herstelt niet de oorspronkelijke EQ-, compressor- of effectinstellingen uit een gemasterde opname. Hij maakt een benaderende klankanalyse en startinstellingen.
- De DSP en interface zijn experimenteel. Een geslaagde build is niet hetzelfde als een professionele, gevalideerde vocal chain.
- Na compilatie moet de VST3 nog in Cubase 13 worden gescand en getest. Dit project bevat geen ondertekende Windows-installer; de workflow levert een VST3-bundle als ZIP.
