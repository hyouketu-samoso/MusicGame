# MusicGame

Unreal Engineを使用したチーム制作の音楽ゲームプロジェクトです。

---

## プロジェクト概要

- **Engine:** Unreal Engine 5.6
- **Language:** C++
- **Version Control:** Git / GitHub
- **Repository:** MusicGame

### 主なディレクトリ・ファイル

- `Config/` — Unreal Engineの設定ファイル
- `Content/` — Blueprint、Level、Material、Texture、Soundなどのアセット
- `Source/` — C++ソースコード
- `MusicGame.uproject` — Unreal Engineプロジェクトファイル

---

## Git運用ルール

### ブランチ運用

`master` はチーム全体の統合用ブランチです。各メンバーは `master` 上で直接作業せず、自分の作業ブランチを使用してください。

作業ブランチの例：

```text
feat/itsuki
```

必要に応じて、機能単位のブランチに分けても構いません。

```text
feat/itsuki-note
feat/itsuki-ui
feat/itsuki-rhythm
```

### masterについて

次の操作は禁止です。

- `master` への直接 commit
- `master` への直接 push
- `master` 上での直接作業
- `master` への merge

`master` への最終的な統合は、原則としてリードプログラマーが担当します。

### その他のGitルール

- force push と rebase は行わない
- 他メンバーの変更を許可なく削除しない
- commit と push は、依頼者から明示的に依頼された場合のみ行う
- 作業内容は作業前後に `git status` で確認し、変更内容を説明する

### コミットメッセージ

commitを依頼された場合は、変更内容に合わせて次の形式を使用します。

```text
[Feat] 新規機能
[Fix] バグ修正
[Refactor] 内部整理
[Docs] コメント・ドキュメント
[Chore] Git設定や雑務
```

## Unreal Engine用 アセット命名ルール

名前を見ただけで何のデータか分かるようにします。基本は **大文字プレフィックス + `_` + パスカルケース** で統一します。

### 基本形

```text
種類_名前
種類_名前_番号
```

例：

```text
BP_Player
BP_NoteTap
WBP_Title
NS_NoteHit_01
S_NoteHit_01
```

### 主に使うプレフィックス

| 種類 | プレフィックス |
| --- | --- |
| Blueprint | `BP_` |
| UI / Widget | `WBP_` |
| StaticMesh | `SM_` |
| SkeletalMesh | `SK_` |
| Texture | `T_` |
| Material | `M_` |
| MaterialInstance | `MI_` |
| NiagaraSystem | `NS_` |
| Sound | `S_` |
| DataAsset | `DA_` |
| DataTable | `DT_` |
| Struct | `ST_` |
| Enum | `E_` |
| InputAction | `IA_` |
| InputMappingContext | `IMC_` |
| LevelSequence | `LS_` |

### 音楽ゲーム用アセットの例

```text
BP_NoteTap
BP_NoteHold
BP_NoteManager
BP_RhythmManager
BP_JudgeManager
BP_SongManager

DA_Song_Starlight
DA_Chart_Starlight_Hard

E_NoteType
E_JudgeType
E_Difficulty

WBP_GameHUD
WBP_Combo
WBP_Judge
WBP_Result

NS_NoteHit
NS_Perfect
NS_BeatPulse

S_Music_Starlight
S_NoteHit_01

T_Jacket_Starlight

IA_Lane_01
IA_Lane_02
IA_Lane_03
IA_Lane_04
```

### 最低限守ること

- プレフィックスを必ず付け、大文字で書く
- `_` の後ろはパスカルケースにする
- Blueprint は `BP_`、UI は `WBP_`、Niagara は `NS_` を使う
- Texture は `T_`、Material は `M_`、Sound は `S_` を使う
- データには `DA_` / `DT_`、Inputには `IA_` / `IMC_` を使う
- 番号は末尾に `_01`, `_02`, `_03` の形式で付ける

### 外部アセット

FabやMarketplace素材は、元データを直接リネームしないでください。使用するものをコピーしてから、自分たちの命名ルールに合わせてください。

```text
Explosion01
↓
NS_NoteHit_01
```

名前だけで「何に使うデータか」が分かる状態を目標にします。

## サウンド用 命名ルール

名前を見ただけで「何の音か・どこで使うか」が分かるようにします。

基本形式：

```text
種類_用途_名前_番号
```

### プレフィックス

| 種類 | プレフィックス |
| --- | --- |
| 楽曲・ゲーム中BGM | `BGM_` |
| 効果音 | `SE_` |
| UI効果音 | `UISE_` |
| ボイス | `VO_` |
| ジングル | `JNG_` |
| 環境音 | `AMB_` |

Unreal上で加工用アセットを作る場合は、次のプレフィックスを使います。

| 種類 | プレフィックス |
| --- | --- |
| SoundCue | `SC_` |
| MetaSound | `MS_` |

### 例

```text
BGM_Title
BGM_Game_Starlight
BGM_Result

SE_NoteHit_01
SE_NoteHit_02
SE_Perfect
SE_Miss
SE_Countdown
SE_FeverStart

UISE_Select
UISE_Decide
UISE_Cancel

VO_Count_01
VO_Count_02

JNG_GameClear
JNG_NewRecord

AMB_Crowd
AMB_StageNoise
```

### 音楽ゲームでよく使うサウンド

```text
BGM_曲名

SE_NoteHit_01
SE_NoteHit_02
SE_HoldStart
SE_HoldEnd
SE_Flick
SE_Perfect
SE_Great
SE_Miss
SE_Combo
SE_FeverStart

UISE_Select
UISE_Decide
UISE_Back
UISE_Pause

JNG_GameStart
JNG_GameClear
JNG_Result
```

### Unrealに入れた後

元の音源をもとに、Unreal上で作るアセットには次の名前を付けます。

| アセット | 名前の例 |
| --- | --- |
| 元の音源 | `SE_NoteHit_01` |
| SoundCue | `SC_NoteHit` |
| MetaSound | `MS_NoteHit` |

### 最低限守ること

- BGMは `BGM_`、効果音は `SE_`、UI音は `UISE_` を使う
- ボイスは `VO_`、ジングルは `JNG_`、環境音は `AMB_` を使う
- 名前部分はパスカルケースにする
- 同じ音の差分は末尾に `_01`, `_02`, `_03` を付ける
- `new`, `final`, `final2` などの曖昧な名前は使わない

```text
× Hit_final2
○ SE_NoteHit_02
```

誰が見ても、その音がどこで使われるか分かる名前にしてください。

---

## 作業開始時

作業前にブランチと作業ツリーの状態を確認してください。

```bash
git branch --show-current
git status
```

`feat/itsuki` など、作業用ブランチであることを確認してから作業を始めてください。`master` 上にいる場合は作業せず、担当者に確認してください。

## 作業終了時

作業後にも変更内容を確認してください。

```bash
git status
```

変更したファイルと内容をチームに共有してください。commitやpushは、明示的に依頼された場合のみ行ってください。
