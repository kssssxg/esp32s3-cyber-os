import java.awt.Desktop;

int fontNumber = -1;
String fontName = "1";
String fontType = ".otf";
int fontSize = 8;
int displayFontSize = 8;
boolean createHeaderFile = true;
boolean openFolder = true;

static final int[] unicodeBlocks = {
  0x0021, 0x007E,
};

int[] specificUnicodes;

int firstUnicode = 0;
int lastUnicode  = 0;

PFont myFont;
PrintWriter logOutput;

void setup() {
  loadSpecificUnicodesFromFile();
  
  logOutput = createWriter("FontFiles/System_Font_List.txt"); 
  size(1000, 800);
  
  String[] fontList = PFont.list();
  printArray(fontList);
  
  for (int x = 0; x < fontList.length; x++)
  {
    logOutput.print("[" + x + "] ");
    logOutput.println(fontList[x]);
  }
  logOutput.flush();
  logOutput.close();
  
  if (fontNumber >= 0)
  {
    fontType = "";
  }
  
  char[]   charset;
  int  index = 0, count = 0;
  
  int blockCount = unicodeBlocks.length;
  
  for (int i = 0; i < blockCount; i+=2) {
    firstUnicode = unicodeBlocks[i];
    lastUnicode  = unicodeBlocks[i+1];
    if (lastUnicode < firstUnicode) {
      delay(100);
      System.err.println("ERROR: Bad Unicode range secified, last < first!");
      System.err.print("first in range = 0x" + hex(firstUnicode, 4));
      System.err.println(", last in range  = 0x" + hex(lastUnicode, 4));
      while (true);
    }
    count += (lastUnicode - firstUnicode + 1);
  }
  
  count += specificUnicodes.length;
  
  println();
  println("=====================");
  println("Creating font file...");
  println("Unicode blocks included     = " + (blockCount/2));
  println("Specific unicodes included  = " + specificUnicodes.length);
  println("Total number of characters  = " + count);
  
  if (count == 0) {
    delay(100);
    System.err.println("ERROR: No Unicode range or specific codes have been defined!");
    while (true);
  }
  
  charset = new char[count];
  
  for (int i = 0; i < blockCount; i+=2) {
    firstUnicode = unicodeBlocks[i];
    lastUnicode  =  unicodeBlocks[i+1];
    for (int code = firstUnicode; code <= lastUnicode; code++) {
      charset[index] = Character.toChars(code)[0];
      index++;
    }
  }
  
  for (int i = 0; i < specificUnicodes.length; i++) {
    charset[index] = Character.toChars(specificUnicodes[i])[0];
    index++;
  }
  
  boolean smooth = true;
  myFont = createFont(fontName+fontType, displayFontSize, smooth, charset);
  
  fill(0, 0, 0);
  textFont(myFont);
  
  int margin = displayFontSize;
  translate(margin/2, margin);
  
  int gapx = displayFontSize*10/8;
  int gapy = displayFontSize*10/8;
  index = 0;
  fill(0);
  
  textSize(displayFontSize);
  
  for (int y = 0; y < height-gapy; y += gapy) {
    int x = 0;
    while (x < width) {
      int unicode = charset[index];
      float cwidth = textWidth((char)unicode) + 2;
      if ( (x + cwidth) > (width - gapx) ) break;
      text(new String(Character.toChars(unicode)), x, y);
      x += cwidth;
      index++;
      if (index >= count) break;
    }
    if (index >= count) break;
  }
  
  PFont font;
  font = createFont(fontName+fontType, fontSize, smooth, charset);
  
  println("Created font " + fontName + str(fontSize) + ".vlw");
  
  String fontFileName = "FontFiles/" + fontName + str(fontSize) + ".vlw";
  
  try {
    print("Saving to sketch FontFiles folder... ");
    OutputStream output = createOutput(fontFileName);
    font.save(output);
    output.close();
    println("OK!");
    delay(100);
    String path = sketchPath();
    if(openFolder){
    }
    System.err.println("All done! Note: Rectangles are displayed for non-existant characters.");
  }
  catch(IOException e) {
    println("Doh! Failed to create the file");
  }
  
  if(!createHeaderFile) return;
  try{
    print("saving header file to FontFile folder...");
    InputStream input = createInputRaw(fontFileName);
    PrintWriter output = createWriter("FontFiles/" + fontName + str(fontSize) + ".h");
    output.println("#include <pgmspace.h>");
    output.println();
    output.println("const uint8_t " + fontName + str(fontSize) + "[] PROGMEM = {");
    int i = 0;
    int data = input.read();
    while(data != -1){
      output.print("0x");
      output.print(hex(data, 2));
      if(i++ < 15){
        output.print(", ");
      } else {
        output.println(",");
        i = 0;
      }
      data = input.read();
    }
    output.println("\n};");
    output.close();
    input.close();
    println("C header file created.");
  } catch(IOException e){
    println("Failed to create C header file");
  }
}

void loadSpecificUnicodesFromFile() {
  String[] lines = loadStrings("1.txt");
  if (lines == null) {
    System.err.println("错误：找不到 data/1.txt 文件，将使用空数组。");
    specificUnicodes = new int[0];
    return;
  }
  StringBuilder sb = new StringBuilder();
  for (String line : lines) {
    sb.append(line.trim());
  }
  String content = sb.toString();
  String[] parts = content.split(",");
  java.util.ArrayList<Integer> list = new java.util.ArrayList<Integer>();
  for (String p : parts) {
    p = p.trim();
    if (p.length() == 0) continue;
    if (p.startsWith("0x") || p.startsWith("0X")) {
      p = p.substring(2);
    }
    try {
      int code = Integer.parseInt(p, 16);
      list.add(code);
    } catch (NumberFormatException e) {
      System.err.println("无法解析十六进制数：" + p);
    }
  }
  specificUnicodes = new int[list.size()];
  for (int i = 0; i < list.size(); i++) {
    specificUnicodes[i] = list.get(i);
  }
  println("成功加载 " + specificUnicodes.length + " 个特定 Unicode 码点。");
}

void draw() {
}