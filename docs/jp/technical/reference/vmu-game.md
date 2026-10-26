# VMUのゲーム

[日本語ドキュメント](../../README.md) | [English](../../../en/technical/reference/vmu-game.md) | [VMUランタイム](vmu-runtime.md) | [リング・りんぐ・ランドのRAM](ring-ring-land-ram.md)

VMUのリング・りんぐ・ランドは、LC86Kのアセンブリで書いたGAMEファイルです。
`pocketstation/src/vmu/` にあるVMU向けのCの関数は、すべてルーチンになりました。
正解は今もCです。
各ルーチンは対応するC関数の記録した呼び出しと照合し、GAME全体は7つのスクリプトすべてで `vmu_runner` とフレームごとに照合します。
このページでは、ソースの構成、すべての関数を管理するマニフェスト、1つのルーチンの確かめ方、ゲーム全体の検査を説明します。
ゲームが動くランタイムの呼び出し規約とRAMは、[VMUランタイム](vmu-runtime.md)のページにあります。

## ソース

ソースは `pocketstation/src/vmu_game/` にあります。

| ファイル | 中身 |
|---|---|
| `game.s` | GAMEの最上位です。ランタイムを取り込み、0x200にVMSヘッダーとアイコンを置き、ルーチンを取り込み、最後に `app_main` とテーブルを置きます。 |
| `callimg.s`、`callbase.inc` | `--call` 用のテストイメージです。同じルーチンに、記録した状態を読み込んで1つのルーチンを呼ぶ小さな土台を付けます。 |
| `<file>.s` | Cのファイル1つにつき1つ（`board.c` には `board.s` など）で、`routines.inc` に並べてあります。 |
| `game.inc` | 共通の定数です。レジスターに入らない引数を置くワークRAMの領域 `ARG_BLOCK` もここにあります。 |
| `manifest.txt` | C関数1つにつき1行です。下で説明します。 |
| `untaken.txt` | どの記録も通らない分岐の行き先と、それでよい理由です。 |
| `Makefile` | `call` はテストイメージと `hw.json` を、`game` は `rrl.vms` と `rrl.map` を、`fixtures` はテスト用プログラムを作ります。 |

テーブル（`tables.s`、`RES1506.BIN` の96ページ）は、ビルド時に `pocketstation/tools/gen_tables.py` が `pocketstation/build/vmu_game/` に作ります。
ディスクから取り出したものは一切コミットしません。
どちらのイメージも `pocketstation/tools/vmu_relax.py` を通します。
これは、3バイトの `CALLF` や `JMPF` を、飛び先が届くところでは2バイトの `CALL`、`JMP`、`BR` に縮めます。
隣の行も `JMPF` である `JMPF` はジャンプテーブルとみなして遠いまま残すので、新しいジャンプテーブルもこの形にします。

## GAMEの配置

- `rt_vectors.s` が0x000〜0x1F5を埋め、0x200にVMSヘッダーとアイコン1つが続いて、0x480で終わります。
- 続いてランタイム、それからすべてのルーチンです。
- `app_main` は、VMUにペットのセーブがなければ `rt_power` で終了します。
  あれば `g_entry` に飛びます。
  `g_entry` はすべてのラインをマスクし、`rt_set_handler` で `g_irq_callback` を登録し、RTCとドックのラインのマスクを外して、`g_main_loop` に入ります。
  どちらも戻りません。
  ランタイムが `app_main` の前にRAMを消しているので、Cの `vmu_ram` の消去は繰り返しません。
- 256バイト境界に置いたテーブルのページと、その直後の `g_bit_rot` です。
  `g_bit_rot` は8ページあり、各ページにはすべてのバイトをビット反転してページ番号の分だけ右に回した値が入っています。
  スプライトの描画ルーチンは、これで1行をxの分だけ `LDC` 1回でずらします。

R3b-3の終了時点で、GAMEは61,684バイトで、フラッシュのバンク0の128ブロックのうち121ブロックです。
ペットのセーブは5ブロックです。
聖剣伝説 LEGEND OF MANAの14ブロックのセーブ2つと合わせても、VMUのユーザー領域200ブロックのうち46ブロックが空いています。

## マニフェスト

`manifest.txt` には、`game.h` のインラインのアクセサーも含めて、C関数1つにつき1行あり、列は8つです。

```
NAME FILE LEVEL ROUTINE|inlined:CALLER ARGS RESULT DESTROYS STATUS
```

- **LEVEL** は `pocketstation/tools/vmu_callgraph.py` が決めます。
  葉がレベル0で、呼ばれる側は必ず呼ぶ側より下にあり、再帰するグループは同じレベルを共有します。
  ルーチンはレベルごとに、呼ばれる側から移植しました。
- **ARGS** は、各引数を `ACC`、`B`、`C`、`TR`（TRH:TRLの16ビット）、`TRB`（TRH:TRL:Bの24ビット）、または `mem:NAME` に対応させます。
  `mem:NAME` は `ARG_BLOCK`（ワークRAMの0x1DB〜0x1E7）の枠で、順番どおり、リトルエンディアンで並びます。
- **RESULT** はレジスター、`mem:` の引数の後ろの `mem:NAME`、ルーチンが読み終えた値の引数に重ねて `ARG_BLOCK+0` に置く `over:NAME`、または `-` です。
- **DESTROYS** は、ルーチンが変えてよいものの一覧です。
  `--call` はルーチンが戻るときにこれらのレジスターを0xA5で汚し、それ以外を変えたルーチンは不合格になります。
- **STATUS** は次のどれかです。
  - `verified`：スクリプトのすべての記録に合格します。
  - `synthetic`：どのスクリプトも通らない関数で、`pocketstation/tests/host/record_synthetic.c` が作った記録に合格します。
  - `reading:callers`：結果がどの枠にも入らないので、呼ぶ側の記録で確かめます。
    `game_frame_info` だけで、その16バイトの結果は13バイトの `ARG_BLOCK` に入りません。
  - `reading:never-returns`：戻らないので、ゲーム全体の実行で確かめます。
    `game_entry`、`game_main_loop`、`game_quit`、`game_dock_quit` です。
    終了やドックを試すスクリプトはないので、後の2つは読んで確かめました。
  - `todo`：移植していません。
    呼ぶ側に埋め込まれる9つのインラインのアクセサーだけがこれです。

終了時点では、182の関数が `verified`、34が `synthetic`、1つが `reading:callers`、4つが `reading:never-returns` です。

自分の `ARG_BLOCK` の枠を持たないのに枠を使うルーチンを呼ぶルーチンは、本体の前後で `g_keep13`、`g_keep9`、`g_keep5`、`g_keep1` を通して枠を保存します。
本体のアドレスをR0:R1に入れて `JMPF` で入るので、ハーネスの影の呼び出しスタックは釣り合ったままです。

## 1つのルーチンの確かめ方

1. `make -C pocketstation vmu-record` で `vmu_runner-record` を作ります。
   生成した層（`pocketstation/tools/gen_record.py`）が、すべての関数の呼び出しを抜き出して記録します。
   記録するのは、RAM、フレームバッファー、引数、結果、ハードウェアの呼び出しです。
   通ったCの分岐の組み合わせごとに、少なくとも1回の呼び出しを残します。
   `test_vmu_game.py` の `recordings` フィクスチャーが、7つのスクリプトと `record_synthetic` を実行して `pocketstation/build/record/rec` に記録します。
2. `make -C pocketstation/harness` と `make -C pocketstation/src/vmu_game call` を実行します。
3. `python pocketstation/tools/call_compare.py FUNCTION` を実行します。
   記録はそれぞれ状態ファイルになります。
   `vmu_harness --call ROUTINE` がそれをバンク1、ワークRAM、XRAMに読み込み、ルーチンを実行して、残った状態を書き出します。
   照合は、バンク1、ワークRAM、XRAMの全バイト、結果、ハードウェアの呼び出しに及びます。
   N個の記録がすべて合えば `ok N` を、合わなければ最初の違いを表示します。
   `--cycles` を付けると、呼び出しごとのサイクル数を表示します。

`test_level_passes[N]` はレベルNのすべてのルーチンでこれを実行します。
`test_level_outcomes_covered[N]` は、そのレベルのすべての分岐の行き先が、合格した記録で通るか `untaken.txt` に載っていることを確かめます。

## VMUでの描画

ゲームは画面そのものであるXRAMに直接描きます。
フレームごとに32列を消して描き直し、その間もLCDはそれを表示しています。
`race` では、ゲームの処理が1フレームの30,766サイクルのうち平均21,277サイクルを使い、その約70%が描画でした。
そのためVMUでは、半分ほどの時間、描きかけの絵が見え、動くスプライトがにじんでいました。
裏バッファーを置くRAMはありません。
バンク1とワークRAMは最後の1バイトまで割り当て済みで、バンク0にはBIOSの変数とスタックがあります。

そこでメインループは、1フレームおきにだけ描きます。
`draw_skip`（バンク1の `0xFC`）は、フレームの消去の前に `cycle16` の次の値の偶奇から設定し、`game_mode_frame` の後で0に戻します。
新しい押下（`btn_new`）が届いたフレームは必ず描くので、カーソルはすぐに動きます。
設定中は何も画面に届かず、最後に描き終えた絵が残ります。
Cでは `set_fb_row` が分岐を使わずにマスクで書き込みを止めます。
アセンブリでは `g_fb_clear`、`g_fb_put_rows`、`g_plot_core` がすぐ戻り、`g_run_fill` と `g_run_xor` は画面の行だけを飛ばします。
ゲームの処理、アニメーションの時間、音、ボタンはこれまでどおり毎フレーム動き、ほかのループは毎フレーム描きます。
`race` は1フレーム平均16,229サイクルになり、逃したティックは320から167に減りました。

`test_skipped_frames_keep_the_last_picture` が飛ばしたフレームを確かめ、`state_compare.py` はVMU向けのCを元のゲームと比べるとき、そのフレームの画素を除きます。

## ゲーム全体の検査

`test_vmu_game.py` の `whole_game` フィクスチャーは、各スクリプトを2回実行します。
1回は `vmu_runner` で、もう1回は、ペットのセーブを入れたフラッシュイメージで `vmu_harness --schedule real --report` の上のGAMEです。

- `test_vmu_game_matches_vmu_runner[SCRIPT]`：7つのスクリプトすべてで、すべてのフレームのダンプ（1 KBの `vmu_ram` と192バイトのフレームバッファー）が一致します。
  `save` には再起動も含まれます。
- `test_vmu_game_pet_save_matches[SCRIPT]`：GAMEが残す `BASLUSP01013` の中身が、同じ方法でパッケージにしたCのセーブと一致し、パッケージのCRCも正しいことを確かめます。
  7つのうち、セーブのバイトを変えるのは `laps`（0x300〜0x30C）だけです。
  `race` と `save` は、参照用のペットに入っているのと同じ値を書きます。
- `test_race_and_menu_frames_fit`：`race`、`menu`、`buttons` で、ティックの間に収まらなかったフレームを、フレーム番号、場面（`mode` フィールド）、理由とともに `pocketstation/build/vmu_game/overruns.txt` に書き出します。
  `changed`（場面が変わった）や `saved`（フラッシュに書き込んだ）の理由があるフレームは認めます。
  理由のないものが、race 167、menu 0、buttons 368（R3b-3の終了時に認めた320と588を1フレームおきの描画で下げ、新しい押下のあるフレームを描くようにした数）を超えるか、盤面の場面3、4、6、7以外で起きると不合格です。
- `test_stack_fits`：一番深いスタックポインターが0xF0以下であることを確かめます。
  race、buttons、laps、saveで0xE1です。
  本物のVMUではBIOSの時計の処理がさらに15〜20バイトほど使いますが、ハーネスはそこを実行しません。
  そのため本物では0xF5ほどまで届きますが、0xFFには収まります。
- `test_game_block_count_recorded`：ブロック数を `pocketstation/build/vmu_game/blocks.txt` に書き出します。
  GAMEは128ブロックより4ブロック以上少なくなければなりません。

`vmu_harness --report --map rrl.map --profile OUT` は、ラベル1つにつき `NAME ALL OVER` の1行も書き出します。
そのラベルから次のラベルまでで実行したサイクル数を、全フレームの合計と、ティックに収まらなかったフレームだけの合計で示します。
ティックに収まらないフレームで何を速くすればよいかは、これで探します。

## ドリームキャストからのインストール

ドリームキャスト版は、CARDAがペットをVMUに送るときにGAMEも入れます。
ディスクのビルドは `pocketstation/build/vmu_game/rrl.vms` を `/cd/VMU/RRL.VMS` にコピーし、これがなければ失敗します。
そのため、先に `make -C pocketstation/src/vmu_game game`（`lom-pocket` イメージ）を実行します。

VMUのBIOSは、GAMEをブロック0からフラッシュの順にそのまま実行します。
KallistiOSはGAMEに空いている一番下のブロックから割り当てますが、それが連続しているとは限りません。
そのため保存処理は、先に `dreamcast/src/card/vmu_game.c` でVMUのFATとディレクトリから判断します。

| VMUの中身 | ペットを送るときの動作 |
|---|---|
| GAMEがなく、ブロック0〜120が空いていて、ペットの分の空きもある | GAMEを `RINGRINGLAND` として書き、それからペットを書く |
| すでに `RINGRINGLAND` がある | ディスクのものとバイトが違えば書き直し、それからペットを書く |
| ほかのGAMEがある | 「Replace the other VMU game?」と聞く。Yesならそれを消して置き換え、Noならスロット選択に戻る |
| ブロック0〜120のどれかに別のファイルがある、または空きが足りない | 「Ring Ring Land requires / 126 Blocks to download.」と表示して断り、ペットは牧場に残す |

GAMEはペットの書き込みの中で書きます（`card_store_kos.c` の `kos_write` が `BASLUSP01013` を書くとき）。
書いた後でFATを読み直し、GAMEがブロック0から途切れずに並んでいることを確かめます。
そうでなければGAMEを消し、送信を失敗にします。

`dreamcast/tests/test_vmu_game_host.py` は判断の処理をホストで確かめます。
`dreamcast/tests/test_pet_transfer_flycast.py` はFlycastでペットを送ります。
送る先は、空のVMU、古いリング・りんぐ・ランド、ほかのミニゲーム（ペットがある場合とない場合）、質問を断る場合、下のブロックにセーブがあるVMUです。
FlycastはVMUのゲームを動かせないので、GAMEで遊ぶのは実機での確認です。
