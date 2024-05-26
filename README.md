# <p align="center">🕹️ Chaos ⚡</p>

<p align="center">
    <img src="Chaos.gif">
</p>

## 📋 Table of contents
<details>
<summary>Click to reveal</summary>

- [About](#-about)
- [Requirements](#-requirements)
- [Building](#-building)
- [Usage](#-usage)
- [Author](#-author)

</details>

## 🔍 About

Chaos is a personal C windows project I've started back in 2022 summer vacations for school.\
It is a [Yuzu](https://en.wikipedia.org/wiki/Yuzu_(emulator)) [Super Smash Bros. Ultimate](https://en.wikipedia.org/wiki/Super_Smash_Bros._Ultimate) cheat application that allows you to increase/decrease the damage taken/shield regeneration of fighters.\
The goal was to learn more about [reverse engineering](https://en.wikipedia.org/wiki/Reverse_engineering) and [Win32](https://en.wikipedia.org/wiki/Windows_API) while working with a game I like.\
It never was to be able to cheat online, it's not possible to use it with nintendo servers but it is somewhat possible to do so with [Parsec](https://parsec.app) as host.\
I've decided to make the repository public due to:
- having refactored/documented all of my code and practices (easy for people to learn from it)
- Super Smash Bros. Ultimate getting its last 13.0.2 update in 2024 after Director Masahiro Sakurai semi-retired in 2023
- Yuzu definitely stopping development at version 1734 after a $2.4 million lawsuit by Nintendo

making Chaos, tested on those latest versions with Windows 11 x64, stable for a while.\
The name Chaos refers to the player responsible for the [Pichugate](https://www.ssbwiki.com/Super_Pichu_cheating_scandal) incident where he modified his setup to buff his swimming goggles Pichu, hence the design.

## 💻 Requirements

You will need:
- [Visual Studio](https://visualstudio.microsoft.com) 2022 (or any other way to build a C file into an executable on Windows)
- Yuzu 1734 (might not work on earlier versions, you'll have to find it online among the DMCAs)
- Super Smash Bros. Ultimate 13.0.1 (probably works on 13.0.2, you'll have to find it on Nintendo Switch NSP/XCI sites)

## 🔧 Building

Setup visual studio and build the project so that the executable is ready to launch.\
Setup yuzu so that it can launch Super Smash Bros. Ultimate (https://github.com/Abd-007/Switch-Emulators-Guide/blob/main/Yuzu.md).

## 🎮 Usage

Chaos was made for 1v1s but can work with more players depending on their fighters (read the app for more info).\
Drag the slider thumbs to modify their respective cheats' values for each fighter from -100% to +100%.\
Click on the hot key buttons to record a key for their respective cheat (only 1 key, no duplicates, blocks them from any other use, detects from any window).\
Click them again (or unfocus from them) to validate.

Everytime you want to play and use Chaos:
- run the Chaos executable
- run Super Smash Bros. Ultimate from Yuzu

Everytime you start a new match on Super Smash Bros. Ultimate:
- click the start button (or release the associated hot key that you defined), the first start will take longer

## 🤝 Author

[Nairod](https://github.com/NairodGH)
