/*
 * BMPv3画像をChDz16色モード用バイナリに変換する
 * Linux用
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

int readFileHeader(FILE * fp);
int readInfoHeader(FILE * fp, int *width, int *height);

static const struct{
  unsigned char r, g, b;
}chdzcolortable[]={
  {0x00, 0x00, 0x00},
  {0x00, 0x55, 0x00},
  {0x00, 0xAA, 0x00},
  {0x00, 0xFF, 0x00},
  {0x00, 0x00, 0xFF},
  {0x00, 0x55, 0xFF},
  {0x00, 0xAA, 0xFF},
  {0x00, 0xFF, 0xFF},
  {0xFF, 0x00, 0x00},
  {0xFF, 0x55, 0x00},
  {0xFF, 0xAA, 0x00},
  {0xFF, 0xFF, 0x00},
  {0xFF, 0x00, 0xFF},
  {0xFF, 0x55, 0xFF},
  {0xFF, 0xAA, 0xFF},
  {0xFF, 0xFF, 0xFF},
  {0x00, 0x00, 0x00}
};

char chdzindex[16] = {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'};

int  main(int argc, char *argv[])
{
  // 変数宣言
  FILE  *src_fileptr, *dst_fileptr,         // 入出力のファイルポインタ
  FILE  *tmp_fileptr[3];                    // 一時ファイルのファイルポインタ
  char  src_name[100], dst_name[100];       // ファイル名
  int   fileheader_size, infoheader_size;   // 二種のヘッダサイズ
  int   width, height, padding;             // 画像の寸法
  unsigned char bytedata,                   // バイトデータ
                bytedata_high,
                bytedata_low;
  //char  *p;
  int opt;                                  // コマンドラインオプション処理用
  bool opt_4x = false;                      // 4倍解像度オプション

  //fprintf(stderr, "BMPv3 image -> ChDz16C nibblemap Converter.\n" ) ;

  // オプション処理
  while ((opt=getopt(argc,argv,"4"))!=-1){  // ハイフンオプションを取得
    switch(opt){
      case '4':
        opt_4x = true;
        break;
      default:
      return 1;
    }

  }
  sprintf(src_name, "%s", argv[optind]);     // ソースBMPのファイル名を取得
  sprintf(dst_name, "%s", argv[optind+1]);   // 出力先のファイル名を取得

  // 入力ファイルをオープン
  if((src_fileptr=fopen(src_name, "rb"))==NULL){
    fprintf( stderr, "ファイル%sが開けません\n", src_name ) ;
    return 1 ;
  }
  // 出力ファイルをオープン
  if((dst_fileptr=fopen(dst_name, "wb"))==NULL){
    fprintf( stderr, "ファイル%sが開けません\n", dst_name ) ;
    return 1 ;
  }
  // 一時ファイルをオープン
  if(opt_4x&&

  // ヘッダ処理
  fileheader_size = readFileHeader( src_fileptr ) ;
  //fprintf(stderr, "File_Header_Size: %d\n", fileheader_size ) ;
  infoheader_size = readInfoHeader( src_fileptr, &width, &height ) ;
  //fprintf(stderr, "Info_Header_Size: %d\n", infoheader_size ) ;
  //fprintf(stderr, "画像サイズ：%d(横)×%d(縦)\n", width, height ) ;
  // 画像形式チェック
  if(width%2)fprintf(stderr,"[わー]横サイズがバイト列形式に適合しません");
  padding=4-(width/2)%4;
  if(padding==4)padding=0;
  //fprintf(stderr, "パディング：%dバイト\n", padding ) ;

  // 行ループ
  for(int i=0; i<height; i++){
    // 行内ループ
    for(int j=0; j<width/2; j++){
      if( fread(&bytedata, 1, 1, src_fileptr) != 1){
        fprintf( stderr, "Data_Read_Error @ i=%d, j=%d\n", i, j) ;
        fclose( src_fileptr ) ;
        exit( 1 ) ;
      }
      bytedata_high = chdzindex[(bytedata & 0xF0) >> 4];
      bytedata_low  = chdzindex[bytedata & 0x0F];
      bytedata      = (bytedata_high << 4) | bytedata_low;
      if(fwrite(&bytedata, sizeof(bytedata), 1, dst_fileptr) < 1){
        fprintf( stderr, "データ書き込み失敗\n" ) ;
      }
    }
    // 行末パディングの読み飛ばし
    for(int j=0; j<padding; j++){
      if( fread(&bytedata, 1, 1, src_fileptr) != 1){
        fprintf( stderr, "Data_Read_Error @ i=%d, j=%d\n", i, j) ;
        fclose( src_fileptr ) ;
        exit( 1 ) ;
      }
    }
  }

  fclose( src_fileptr ) ;
  fclose( dst_fileptr ) ;

  return 0 ;
}

int readFileHeader(FILE *fp)
{
  int   count = 0;
  int   var_long;
  short var_short;
  char  s[10];

  /* BMPシグネチャ "BM" */
  if (fread(s, 2, 1, fp) == 1){
    /*
    if (memcmp(s, "BM", 2) == 0)
      fprintf(stderr, "[BM] BITMAP file\n");
    else{
      */
    if (memcmp(s, "BM", 2) != 0){
      fprintf(stderr, "%s : Not a BMP file\n", s);
      fclose(fp);
      exit(1);
    }
    count += 2;
  }
  //fprintf(stderr, "BITMAPFILEHEADER\n");

  /* ファイルサイズ */
  if (fread(&var_long, 4, 1, fp) == 1) {
    //fprintf(stderr, "  Size          : %d [Byte]\n", var_long);
    count += 4;
  }
  /* 予約領域 */
  if (fread(&var_short, 2, 1, fp) == 1)
    count += 2;

  /* 予約領域 */
  if (fread(&var_short, 2, 1, fp) == 1)
    count += 2;

  /* ファイルの先頭から画像データまでの位置 */
  if (fread(&var_long, 4, 1, fp) == 1) {
    //fprintf(stderr, "  OffBits       : %d [Byte]\n", var_long);
    count += 4;
  }

  return count;
}

int readInfoHeader(FILE *fp, int *width, int *height)
{
  int           count = 0;
  int           var_long, compress = 0;
  short         var_short;
  unsigned char var_char[4];

  /* BITMAPINFOHEADER のサイズ  */
  /* Windows BMPファイルのみ受付  */
  if (fread(&var_long, 4, 1, fp) == 1) {
    count += 4;
    if( var_long != 40 ){
      fprintf(stderr, "Not a Windows BMP file\n");
      fclose( fp ) ;
      exit(1);
    }
  }

  //fprintf(stderr, "BITMAPINFOHEADER\n");
  /* Windows BMP */
  /* 画像データの幅 */
  if (fread(&var_long, 4, 1, fp) == 1) {
    //fprintf(stderr, "  Width         : %d [pixel]\n", var_long);
    *width = var_long;
    count += 4;
  }
  /* 画像データの高さ */
  if (fread(&var_long, 4, 1, fp) == 1) {
    //fprintf(stderr, "  Height        : %d [pixel]\n", var_long);
    *height = var_long;
    count += 4;
  }
  /* プレーン数 (1のみ) */
  if (fread(&var_short, 2, 1, fp) == 1) {
    count += 2;
  }
  /* 1画素あたりのビット数 (1, 4, 8, 24, 32)  */
  /* 4ビットカラーのみ受付          */
  if (fread(&var_short, 2, 1, fp) == 1) {
    //fprintf(stderr, "  BitCount      : %d [bit]\n", var_short);
    if( var_short != 4 ){
      fprintf(stderr, "4bitカラーではありません\n");
      fclose( fp ) ;
      exit(1);
    }
    count += 2;
  }
  /* 圧縮方式  0 : 無圧縮             */
  /*           1 : BI_RLE8 8bit RunLength 圧縮  */
  /*           2 : BI_RLE4 4bit RunLength 圧縮  */
  if (fread(&var_long, 4, 1, fp) == 1) {
    //fprintf(stderr, "  Compression   : %d\n", var_long);
    compress = var_long;
    count += 4;
    if( var_long != 0 ){
      fprintf(stderr, "非圧縮モードではありません\n");
      fclose( fp ) ;
      exit(1);
    }
  }
  /* 画像データのサイズ */
  if (fread(&var_long, 4, 1, fp) == 1) {
    //fprintf(stderr, "  SizeImage     : %d [Byte]\n", var_long);
    count += 4;
  }
  /* 横方向解像度 (Pixel/meter) */
  if (fread(&var_long, 4, 1, fp) == 1) {
    //fprintf(stderr, "  XPelsPerMeter : %d [pixel/m]\n", var_long);
    count += 4;
  }
  /* 縦方向解像度 (Pixel/meter) */
  if (fread(&var_long, 4, 1, fp) == 1) {
    //fprintf(stderr, "  YPelsPerMeter : %d [pixel/m]\n", var_long);
    count += 4;
  }
  /* 使用色数 */
  if (fread(&var_long, 4, 1, fp) == 1) {
    //fprintf(stderr, "  ClrUsed       : %d [color]\n", var_long);
    count += 4;
  }
  /* 重要な色の数 0の場合すべての色 */
  if (fread(&var_long, 4, 1, fp) == 1)
    count += 4;

  /* カラーパレット取得 */
  for(int i=0; i<16; i++){
    if(fread(&var_char, 1, 4, fp) == 4){
      //fprintf(stderr, "pallet[%d]:%x %x %x\n", i, var_char[0], var_char[1], var_char[2]);
      // パレットテーブルの検索
      for(int j=0; j<16; j++){
        if(chdzcolortable[j].r==var_char[2] && chdzcolortable[j].g==var_char[1] &&
            chdzcolortable[j].b==var_char[0]){
          // インデックスを登録
          chdzindex[i]=j;
          //fprintf(stderr, "chdzindex[%x]=%x\n", i, chdzindex[i]);
        }
      }
    }
  }

  return count;
}

