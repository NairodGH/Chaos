# <p align="center">🕹️ Chaos ⚡</p>

<p align="center">
    <img src="Chaos.gif">
</p>

## 📋 Table of contents
<details>
<summary>Click to reveal</summary>

- [Important](#-important)
- [About](#-about)
- [Requirements](#-requirements)
- [Building](#-building)
- [Usage](#-usage)
- [Author](#-author)

</details>

## ⚠️ Important

[Nintendo](https://www.nintendo.com/) and its affiliates own all rights, titles, and interests to the Nintendo Switch system, its software, game titles, trademarks, and related intellectual property.\
This project is strictly for educational and research purposes.\
It does not support, promote, or encourage cheating in online play, bypassing technological protection measures, cracking or distributing copyrighted games, or using discontinued or unauthorized emulators or services.\
You cannot use this application in online play on Nintendo's or the emulators' servers.
Any use of this project that infringes on Nintendo’s rights or violates applicable law is neither intended nor endorsed.

## 🔍 About

Chaos is a personal C windows project I've started back in 2022 summer vacations for school.\
It is a [Nintendo Switch emulator](https://en.wikipedia.org/wiki/Nintendo_Switch_emulation) [Super Smash Bros. Ultimate](https://en.wikipedia.org/wiki/Super_Smash_Bros._Ultimate) cheat application that allows you to increase/decrease the damage taken/shield regeneration of fighters.\
The goal was to learn more about [reverse engineering](https://en.wikipedia.org/wiki/Reverse_engineering) and [Win32](https://en.wikipedia.org/wiki/Windows_API) while working with a game I like.\
I've decided to make the repository public due to:
- having refactored/documented all of my code and practices (easy for people to learn from it)
- Super Smash Bros. Ultimate getting its last updates in 2025 after director Masahiro Sakurai semi-retired in 2023
- most Switch emulators being discontinued

making Chaos, tested on those emulators' latest versions with Windows 11 x64, stable for a while.\
The name Chaos refers to the player responsible for the [Pichugate](https://www.ssbwiki.com/Super_Pichu_cheating_scandal) incident where he modified his setup to buff his swimming goggles Pichu, hence the design.

## 💻 Requirements

You will need:
- A supported emulator: whether [Yuzu](https://yuzu-emulator.com/), [Ryujinx](https://ryujinx.io/) or their derivatives (works on sudachi, a fork of yuzu, so I assume it works on its others)
- Super Smash Bros. Ultimate 13.0.x

If for some reason the [latest released executable](https://github.com/NairodGH/Chaos/releases) doesn't work on your machine, you can build it from source with [Visual Studio](https://visualstudio.microsoft.com) using the provided sln file (or any other way to build a C file into an executable on Windows).

## 🔧 Building

Setup visual studio and build the project so that the executable is ready to launch.\
Setup yuzu so that it can launch Super Smash Bros. Ultimate.

## 🎮 Usage

Select your SSBU version using the top list, it defaults to the latest supported version and will not work if you select another than the one you're running.\
Chaos was made for 1v1s but can work with more players depending on their fighters (read the app for more info).\
Drag the slider thumbs to modify their respective cheats' values for each fighter from -100% to +100%.\
Click on the hot key buttons to record a key for their respective cheat (only 1 key, no duplicates, blocks them from any other use, detects from any window).\
Click them again (or unfocus from them) to validate.

Everytime you want to play and use Chaos:
- run the Chaos executable
- run Super Smash Bros. Ultimate from Yuzu

Everytime you start a new match on Super Smash Bros. Ultimate:
- click the start button (or release the associated hot key that you defined), the first start will take longer (more on ryujinx)

## 🤝 Author

[Nairod](https://github.com/NairodGH)
