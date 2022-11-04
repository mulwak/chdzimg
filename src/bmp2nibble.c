/*
 * BMPv3画像をChDz16色モード用バイナリに変換する
 * Linux用
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

int readFileHeader(FILE * fp, int opt_verbose);
int readInfoHeader(FILE * fp,  int opt_verbose, int *width, int *height);
int readColorPallet(FILE *fp, int opt_verbose);
void close_and_exit();

static const struct{
  unsigned char r, g, b;
}chdzcolortable[]={
  {0x00, 0x00, 0x00}, {0x00, 0x55, 0x00}, {0x00, 0xAA, 0x00}, {0x00, 0xFF, 0x00},
  {0x00, 0x00, 0xFF}, {0x00, 0x55, 0xFF}, {0x00, 0xAA, 0xFF}, {0x00, 0xFF, 0xFF},
  {0xFF, 0x00, 0x00}, {0xFF, 0x55, 0x00}, {0xFF, 0xAA, 0x00}, {0xFF, 0xFF, 0x00},
  {0xFF, 0x00, 0xFF}, {0xFF, 0x55, 0xFF}, {0xFF, 0xAA, 0xFF}, {0xFF, 0xFF, 0xFF}
};

char chdzindex[16] = {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'};
FILE  *src_fileptr, *dst_fileptr;          // 入出力のファイルポインタ


int main(int argc, char *argv[])
{
  // 変数宣言
  FILE  *work_fileptr, *tmp_fileptr[4];     // 作業用ポインタ、一時ファイルのファイルポインタ
  char  src_name[100], dst_name[100];       // ファイル名
  int   width, height, padding;             // 画像の寸法
  unsigned char bytedata;                   // バイトデータ
  int opt;                                  // コマンドラインオプション処理用
  bool opt_4x     = false,                  // 4倍解像度オプション
       opt_verbose = false;                 // 冗長メッセージ

  if(opt_verbose)fprintf(stderr, "BMPv3 image -> ChDz16C nibblemap Converter.\n");

  // オプション処理
  while ((opt=getopt(argc,argv,"4v"))!=-1){  // ハイフンオプションを取得
    switch(opt){
      case '4':
        opt_4x = true;                      // 4倍解像度モード
        break;
      case 'v':
        opt_verbose = true;                 // 詳細メッセージ
        break;
      default:
      return -1;
    }

  }

  // ファイル名取得
  sprintf(src_name, "%s", argv[optind]);     // ソースBMPのファイル名を取得
  sprintf(dst_name, "%s", argv[optind+1]);   // 出力先のファイル名を取得

  // 入力ファイルをオープン
  if((src_fileptr=fopen(src_name, "rb"))==NULL){
    fprintf(stderr, "File_Open_Error: %s\n", src_name) ;
    return -1;
  }
  // 出力ファイルをオープン
  if((dst_fileptr=fopen(dst_name, "wb"))==NULL){
    fprintf(stderr, "File_Open_Error: %s\n", dst_name) ;
    fclose(src_fileptr);
    return -1;
  }
  // 一時ファイルをオープン
  if(opt_4x){
    tmp_fileptr[0]=dst_fileptr;
    for(int i=1;i<4;i++){
      if((tmp_fileptr[i]=tmpfile())==NULL){
        fprintf(stderr, "TMP_files_Open_Error\n") ;
        close_and_exit();
      }
    }
  }

  // ファイルヘッダ取得
  if(readFileHeader(src_fileptr,opt_verbose))
    fprintf(stderr,"Read_BITMAPFILEHEADER_Error\n");
  // 情報ヘッダ取得
  if(readInfoHeader(src_fileptr,opt_verbose, &width, &height))
    fprintf(stderr,"Read_BITMAPINFOHEADER_Error\n");
  // カラーパレット取得
  if(readColorPallet(src_fileptr,opt_verbose))
    fprintf(stderr,"Read_ColorPallet_Error\n");
  // 画像寸法
  if(opt_verbose)fprintf(stderr, "Image_Size：%d(H)×%d(V)\n", width, height);
  // 画像形式チェック
  if(height%4)fprintf(stderr,"[わー]縦サイズが4の倍数ではありません");
  if(width%2)fprintf(stderr,"[わー]横サイズがバイト列形式に適合しません");
  // BMPのパディングの大きさを算出
  padding=4-(width/2)%4;
  if(padding==4)padding=0;
  if(opt_verbose)fprintf(stderr, "Padding: %d [Bytes]\n", padding);

  // 行ループ
  for(int i=0; i<height; i++){
    // 4xオプションオンなら、対応する一時ファイルに書き込む
    work_fileptr=opt_4x?tmp_fileptr[i%4]:dst_fileptr;
    // 行内ループ
    for(int j=0; j<width/2; j++){
      if(fread(&bytedata, sizeof(bytedata), 1, src_fileptr) != 1){
        fprintf(stderr, "SRC_Read_Error@ i=%d, j=%d\n", i, j);
        close_and_exit();
      }
      bytedata=chdzindex[((bytedata&0xF0)>>4)]<<4 // 上位ニブル
              |chdzindex[bytedata&0x0F];        // 下位ニブル
      if(fwrite(&bytedata, sizeof(bytedata), 1, work_fileptr) != 1){
        fprintf(stderr, "DST_Write_Error@ i=%d, j=%d\n", i, j);
        close_and_exit();
      }
    }
    // 行末パディングの読み飛ばし
    for(int j=0; j<padding; j++){
      if(fread(&bytedata, sizeof(bytedata), 1, src_fileptr) != 1){
        fprintf(stderr, "Padding_Read_Error@ i=%d, j=%d\n", i, j);
        close_and_exit();
      }
    }
  }

  // 分割出力を統合する
  if(opt_4x){
    // 画面ループ
    for(int i=1;i<4;i++){
      fseek(tmp_fileptr[i],0L,SEEK_SET);                // 先頭に戻る
      // 画面内ループ
      for(int j=0;j<(height*(width/2)/4);j++){
        if(fread(&bytedata, sizeof(bytedata), 1, tmp_fileptr[i]) != 1){
          fprintf(stderr, "TMP_Read_Error@ i=%d, j=%d\n", i, j);
          close_and_exit();
        }
        if(fwrite(&bytedata, sizeof(bytedata), 1, tmp_fileptr[0]) != 1){
          fprintf(stderr, "TMP_Write_Error@ i=%d, j=%d\n", i, j);
          close_and_exit();
        }
      }
    }
  }

  // 終了処理
  fclose(src_fileptr);
  fclose(dst_fileptr);
  return 0;
}

int readFileHeader(FILE *fp, int opt_verbose)
{
  int   tmp_long;
  short tmp_short;
  char  s[10];

  // BMPシグネチャ "BM"
  if (fread(s, 2, 1, fp) == 1){
    if (memcmp(s, "BM", 2) != 0){
      fprintf(stderr, "%s : Not a BMP file\n", s);
      return -1;
    }
  }
  if(opt_verbose)fprintf(stderr, "BITMAPFILEHEADER\n");

  // ファイルサイズ
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  Size          : %d [Byte]\n", tmp_long);
  // 予約領域
  if(fread(&tmp_short, sizeof(tmp_short), 2, fp) != 2)return -1;

  // ファイルの先頭から画像データまでの位置
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  OffBits       : %d [Byte]\n", tmp_long);

  return 0;
}

int readInfoHeader(FILE *fp, int opt_verbose, int *width, int *height)
{
  int           tmp_long;
  short         tmp_short;
  unsigned char tmp_char[4];

  // BITMAPINFOHEADER のサイズ
  // Windows BMPファイルのみ受付
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(tmp_long != 40){
    fprintf(stderr, "Not a Windows BMP file\n");
    return -1;
  }

  if(opt_verbose)fprintf(stderr, "BITMAPINFOHEADER\n");
  // Windows BMP
  // 画像データの幅
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  Width         : %d [pixel]\n", tmp_long);
  *width = tmp_long;
  // 画像データの高さ
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  Height        : %d [pixel]\n", tmp_long);
  *height = tmp_long;
  // プレーン数 (1のみ)
  if(fread(&tmp_short, sizeof(tmp_short), 1, fp) != 1)return -1;
  // 1画素あたりのビット数 (1, 4, 8, 24, 32)
  // 4ビットカラーのみ受付
  if(fread(&tmp_short, sizeof(tmp_short), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  BitCount      : %d [bit]\n", tmp_short);
  if(tmp_short != 4){
    fprintf(stderr, "4bitカラーではありません\n");
    return -1;
  }
  /*
   * 圧縮方式  0 : 無圧縮
   *           1 : BI_RLE8 8bit RunLength 圧縮
   *           2 : BI_RLE4 4bit RunLength 圧縮
   */
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  Compression   : %d\n", tmp_long);
  if(tmp_long != 0){
    fprintf(stderr, "非圧縮モードではありません\n");
    return -1;
  }
  // 画像データのサイズ
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  SizeImage     : %d [Byte]\n", tmp_long);
  // 横方向解像度 (Pixel/meter)
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  XPelsPerMeter : %d [pixel/m]\n", tmp_long);
  // 縦方向解像度 (Pixel/meter)
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  YPelsPerMeter : %d [pixel/m]\n", tmp_long);
  // 使用色数
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;
  if(opt_verbose)fprintf(stderr, "  ClrUsed       : %d [color]\n", tmp_long);

  // 重要な色の数 0の場合すべての色
  if(fread(&tmp_long, sizeof(tmp_long), 1, fp) != 1)return -1;

  return 0;
}

// カラーパレット取得
int readColorPallet(FILE *fp, int opt_verbose){
  unsigned char tmp_char[4];
  if(opt_verbose)fprintf(stderr, "BMP_Pallet          -> chdzindex\n");
  for(int i=0; i<16; i++){
    if(fread(&tmp_char, sizeof(tmp_char[0]), 4, fp) != 4)return -1;
    if(opt_verbose)fprintf(stderr, "  [%2d] = (%2x,%2x,%2x)", i, tmp_char[0], tmp_char[1], tmp_char[2]);
    // パレットテーブルの検索
    for(int j=0; j<16; j++){
      if(chdzcolortable[j].r==tmp_char[2]&&
         chdzcolortable[j].g==tmp_char[1]&&
         chdzcolortable[j].b==tmp_char[0]){
        // インデックスを登録
        chdzindex[i]=j;
        if(opt_verbose)fprintf(stderr, " ->   [%2x] = %2x", i, chdzindex[i]);
      }
    }
    if(opt_verbose)fprintf(stderr, "\n");
  }
  return 0;
}

// ファイルI/Oエラー用
void close_and_exit(){
  fclose(src_fileptr);
  fclose(dst_fileptr);
  exit(1);
}

