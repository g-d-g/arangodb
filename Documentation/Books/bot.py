import sys
import re
import os

def walk_on_files(dirpath):
  for root, dirs, files in os.walk(dirpath):
    for file in files:
      if file.endswith(".md"):
        if file == "SUMMARY.md":
          pass
        else:  
          full_path= os.path.join(root, file)
          f=open(full_path,"rU")
          print "checking file: " + full_path + file
          textFile=f.read()
          f.close()
          findStartCode(textFile,full_path)
          replaceCode(full_path)
  return

def findStartCode(textFile,full_path):
  match = re.findall(r'@startDocuBlock\s*(\w+)', textFile)
  if match:      
    for find in match:
      textToReplace=getTextFromSourceFile(find, full_path)

def getTextFromSourceFile(searchText, full_path):
  f=open("allComments.txt", 'rU')
  s=f.read()
  match = re.search(r'@startDocuBlock\s+'+ searchText + "(?:\s+|$)" +'(.+?)@endDocuBlock', s,re.DOTALL)
  if match:
    textExtracted = match.group(1)
    textExtracted = textExtracted.replace("<br />","\n")
    replaceText(textExtracted, full_path, searchText)

def replaceText(text, pathOfFile, searchText):
  f=open(pathOfFile,"rU")
  if f:
    s=f.read()
  f.close()
  f=open(pathOfFile,'w')
  replaced = re.sub("@startDocuBlock\s+"+ searchText + "(?:\s+|$)",text,s)
  f.write(replaced)
  f.close()
  

def replaceCode(pathOfFile):
  f=open(pathOfFile,"rU")
  if f:
    s=f.read()
  f.close()
  f=open(pathOfFile,'w')
  lines = re.sub(r"<!--(\s*.+\s)-->","",s)
  #HTTP API changing code
  lines = re.sub(r"@brief(.+)",r"\g<1>", lines)
  lines = re.sub(r"@RESTHEADER{([\s\w\/\_{}-]*),([\s\w-]*)}", r"###\g<2>\n`\g<1>`", lines)
  lines = lines.replace("@RESTDESCRIPTION","")
  lines = lines.replace("@RESTURLPARAMETERS","**URL Parameters**\n")
  lines = lines.replace("@RESTURLPARAMS","**URL Parameters**\n")
  lines = lines.replace("@RESTQUERYPARAMS","**Query Parameters**\n")
  lines = lines.replace("@RESTQUERYPARAMETERS","**Query Parameters**\n")
  lines = lines.replace("@RESTHEADERPARAMS","**Header Parameters**\n")
  lines = lines.replace("@RESTHEADERPARAMETERS","**Header Parameters**\n")
  lines = lines.replace("@RESTBODYPARAMS","**Body Parameters**\n")
  lines = lines.replace("@RESTBODYPARAMETERS","**Body Parameters**\n")
  lines = lines.replace("@RESTRETURNCODES","**Return Codes**\n")
  lines = lines.replace("@RESTURLPARAM", "@RESTPARAM")
  lines = lines.replace("@PARAMS", "**Parameters**\n")
  lines = lines.replace("@PARAM", "@RESTPARAM")
  lines = lines.replace("@RESTHEADERPARAM", "@RESTPARAM")
  lines = lines.replace("@RESTQUERYPARAM", "@RESTPARAM")
  lines = lines.replace("@RESTBODYPARAM", "@RESTPARAM")
  lines = re.sub(r"@RESTPARAM{([\s\w\-]*),([\s\w\_\|-]*),\s[optional]}", r"* *\g<1>* (\g<3>):", lines)
  lines = re.sub(r"@RESTPARAM{([\s\w-]*),([\s\w\_\|-]*),\s*(\w+)}", r"* *\g<1>*:", lines)
  lines = re.sub(r"@RESTRETURNCODE{(.*)}", r"* *\g<1>*:", lines)
  lines = re.sub(r"@RESTBODYPARAMS{(.*)}", r"*(\g<1>)*", lines)
  lines = lines.replace("@EXAMPLES","**Examples**")
  # Error codes replace
  lines = re.sub(r"- (\w+):\s*@LIT{(.+)}", r"\n*\g<1>* - **\g<2>**:", lines)
  f.write(lines)
  f.close()



if __name__ == '__main__':
    path = ["Documentation/Books/Users"]
    for i in path:
      dirpath = os.path.abspath(os.path.join(os.path.dirname( __file__ ), os.pardir,"ArangoDB/../../"+i))  
    walk_on_files(dirpath)