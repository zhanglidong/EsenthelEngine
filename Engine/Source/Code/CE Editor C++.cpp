﻿/******************************************************************************/
#include "stdafx.h"
namespace EE{
static Int Compare(C Edit::CodeEditor::BuildFile &a, C Edit::CodeEditor::BuildFile &b)
{
   return ComparePath(a.src_proj_path, b.src_proj_path);
}
namespace Edit{
/******************************************************************************/
#define MERGE_HEADERS 0 // this made almost no difference on an SSD Disk
/******************************************************************************/
// TODO: VSRun doesn't include build configuration, Debug/Release, DX10+/GL, 32/64bit
  void CodeEditor::VS     (C Str &command,   Bool console,   Bool hidden              ) {if(console ? !build_process.create(devenv_path, command, hidden) : !Run(devenv_path, command, hidden))Error(S+"Error launching:\n\""+devenv_path+'"');}
//void CodeEditor::VSClean(C Str &project                                             ) {VS(S+"\""+NormalizePath(MakeFullPath(project))+"\" /Clean", true, true);} unused
  void CodeEditor::VSBuild(C Str &project, C Str &config , C Str &platform, C Str &out) {VS(VSBuildParams(project, config, platform, out), true, true);}
  void CodeEditor::VSRun  (C Str &project                                 , C Str &out) {VS(S+"/RunExit \""+NormalizePath(MakeFullPath(project))+'"'+(out.is() ? S+" /Out \""+NormalizePath(MakeFullPath(out))+'"' : S), false, false);}
  void CodeEditor::VSOpen (C Str &project, C Str &file                                ) {VS(S+'"'+NormalizePath(MakeFullPath(project))+'"'+(file.is() ? S+"/command open \""+file+'"' : S), false, false);} // for unknown reasons this opens file and also "open file window"
//void CodeEditor::VSDebug(C Str &exe                                                 ) {VS(S+"/DebugExe \""+exe+'"', false, false);} // it doesn't load any project/sources, only pure .exe, unused
/******************************************************************************/
void CodeEditor::validateDevEnv()
{
   devenv_version=-1;
   devenv_express= false;
   devenv_path.clear();
   if(vs_path.is())
   {
      if(!FExistSystem(devenv_path))
      {
         // don't try .com as 'ConsoleProc.kill' does not work on it
       /*devenv_path   =Str(vs_path).tailSlash(true)+"Common7\\IDE\\devenv.com"; // first try devenv (pro, ..) com
         devenv_com    =true;
         devenv_express=false;
         if(!FExistSystem(devenv_path))*/
         {
            devenv_path   =Str(vs_path).tailSlash(true)+"Common7\\IDE\\devenv.exe"; // try devenv (pro, ..) exe
            devenv_com    =false;
            devenv_express=false;
            if(!FExistSystem(devenv_path))
            {
             /*devenv_path   =Str(vs_path).tailSlash(true)+"Common7\\IDE\\VCExpress.com"; // try express com
               devenv_com    =true;
               devenv_express=true;
               if(!FExistSystem(devenv_path))*/
               {
                  devenv_path   =Str(vs_path).tailSlash(true)+"Common7\\IDE\\VCExpress.exe"; // try express exe
                  devenv_com    =false;
                  devenv_express=true;
                  if(!FExistSystem(devenv_path))
                  {
                     devenv_path   =Str(vs_path).tailSlash(true)+"Common7\\IDE\\WDExpress.exe"; // try express exe 2012 for Windows Desktop
                     devenv_com    =false;
                     devenv_express=true;
                     if(!FExistSystem(devenv_path))
                     {
                        if(FExistSystem(Str(vs_path).tailSlash(true)+"Common7\\IDE\\VSWinExpress.exe"))Error("This edition of Visual Studio does not support creating Native Applications.\nPlease install Visual Studio for Windows Desktop."); // check for "Visual Studio 2012 for Windows 8"
                        devenv_path.clear();
                     }
                  }
               }
            }
         }
      }
      if(devenv_path.is())devenv_version=EE::FileVersion(devenv_path);
   }
}
/******************************************************************************/
void CodeEditor::setVSPath         (C Str &path) {          vs_path=path; options.          vs_path.set(path, QUIET); validateDevEnv();}
void CodeEditor::setNBPath         (C Str &path) {    netbeans_path=path; options.    netbeans_path.set(path, QUIET);                  }
void CodeEditor::setASPath         (C Str &path) {      android_sdk=path; options.      android_sdk.set(path, QUIET);                  }
void CodeEditor::setANPath         (C Str &path) {      android_ndk=path; options.      android_ndk.set(path, QUIET);                  }
void CodeEditor::setJDKPath        (C Str &path) {         jdk_path=path; options.         jdk_path.set(path, QUIET);                  }
void CodeEditor::setAndroidCertPath(C Str &path) {android_cert_file=path; options.android_cert_file.set(path, QUIET);                  }
void CodeEditor::setAndroidCertPass(C Str &pass) {android_cert_pass=pass; options.android_cert_pass.set(pass, QUIET);                  }
void CodeEditor::setAppleTeamID    (C Str &id  ) {    apple_team_id=id  ; options.    apple_team_id.set(id  , QUIET);                  }
/******************************************************************************/
static Bool ValidPackage(C Str &name)
{
   if( !name.is())return false; // empty
   REPA(name)
   {
      Char c=name[i];
      if(!((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='-' || c=='.'))return false;
   }
   return true;
}
static Bool ValidatePackage()
{
   Str name=CE.cei().appPackage();
   if(!ValidPackage(name))
   {
      CE.cei().appInvalidProperty(name.is() ? "Invalid Package Name of the active application.\nThe name parts may contain uppercase or lowercase letters 'A' through 'Z', numbers, hyphens '-' and underscores '_' only." : "Package name of the active application was not specified.");
      return false;
   }
   return true;
}
static Str AndroidPackage(C Str &name) {return Replace(name, '-', '_');} // Android does not support '-' but supports '_'
Bool CodeEditor::verifyVS()
{
   if(build_exe_type==EXE_UWP && !ValidatePackage())return false;
   if(devenv_version.x<=0)validateDevEnv();
   Str message; if(CheckVisualStudio(devenv_version, &message, false))return true; // we can't check the minor version because EXE have it always set to 0 (last tested on VS 2017 regular+preview)
   options.activatePaths(); return Error(message);
}
Bool CodeEditor::verifyXcode()
{
   if(build_exe_type==EXE_IOS)
   {
      if(!ValidatePackage())return false;
      if(!apple_team_id.is()){options.activateCert(); Error("Apple Team ID was not specified."); return false;}
   }
   return true;
}
Bool CodeEditor::verifyAndroid()
{
   Str error;

   if(! android_sdk  .is())error.line(2)+="The path to Android SDK has not been specified.";else
   if(!     adbPath().is())error.line(2)+="Can't find \"adb\" tool in Android SDK.";else
   if(!zipalignPath().is())error.line(2)+="Can't find \"zipalign\" tool in Android SDK.";

   if(! android_ndk  .is()      )error.line(2)+="The path to Android NDK has not been specified.";else
   if(Contains(android_ndk, ' '))error.line(2)+="Android NDK will not work if it's stored in a path with spaces.\nPlease move the Android NDK folder to a path without spaces and try again.";else
   if(!ndkBuildPath().is()      )error.line(2)+="Can't find \"ndk-build\" tool in Android NDK.";

   if(error.is())
   {
      options.activatePaths();
      return Error(error);
   }
   if(!ValidatePackage())return false;

   Str key=cei().appGooglePlayLicenseKey();
   if(Contains(key, '"') || Contains(key, '\n') || Contains(key, '\\')){cei().appInvalidProperty("App License Key is invalid."); return false;}

   if(cei().appAndroidExpansion() && !key.is()){cei().appInvalidProperty("Auto-downloading Android Expansion Files requires setting Android License Key."); return false;}

   return true;
}
Bool CodeEditor::verifyLinuxMake    () {return true;}
Bool CodeEditor::verifyLinuxNetBeans()
{
   if(!FExistSystem(Str(netbeans_path).tailSlash(true)+"bin/netbeans"))
   {
      options.activatePaths();
      return Error("The path to NetBeans has not been specified or is invalid.\nPlease select the path to a valid installation of NetBeans with C++ support.");
   }
   return true;
}
/******************************************************************************/
static Str BinPath(Bool allow_relative=true)
{
   Str path=GetPath(App.exe())+"/Bin/";
   if(CE.options.export_path_mode() && allow_relative)path=GetRelativePath(CE.build_path, path);
   return path;
}
static Str AppName(C Str &str, EXE_TYPE exe_type)
{
   Str out;
   FREPA(str)
   {
      Char c=str[i];
      if(exe_type==EXE_APK)
      {
         if(c=='&')c='n'; // android will fail when uploading '&' APK to device
      }
      if(c==':')
      {
         if(str[i+1]==' ' && CharType(str[i-1])==CHART_CHAR)out+=' '; // check for "main: sub" -> "main - sub"
         out+='-';
      }else out+=c;
   }
   return CleanFileName(out);
}
Bool CodeEditor::getBuildPath(Str &build_path, Str &build_project_name, C Str *app_name)
{
   build_project_name=AppName(app_name ? *app_name : cei().appName(), build_exe_type); if(!build_project_name.is())return false;
   build_path        =NormalizePath(MakeFullPath(projects_build_path));

   build_path.tailSlash(true)+=build_project_name;
   build_path.tailSlash(true); // !! 'build_path' may get deleted in 'clean' !!
   return true;
}
/******************************************************************************/
Bool CodeEditor::generateTXT(Memc<Message> &msgs)
{
   Bool ok=true;
   Memc<Str> files;
   FREPA(sources)
   {
      Source &source=sources[i];
      if(source.active)
      {
         Str src_proj_path=cei().sourceProjPath(source.loc.id);
         if( src_proj_path.is())
         {
            if(files.binaryInclude(src_proj_path, ComparePathCI))
            {
               Str name=build_path+"Source\\"+src_proj_path+".txt";
               FCreateDirs(GetPath(name));
               source.saveTxt(name);
            }else
            {
               msgs.New().error(S+"Multiple files with the same name \""+src_proj_path+'"', &source, -1);
               ok=false;
            }
         }
      }
   }
   return ok;
}
/******************************************************************************/
Bool CodeEditor::verifyBuildFiles(Memc<Message> &msgs)
{
   Bool ok=true;

   build_files.clear();
   FREPA(sources)
   {
      Source &source=sources[i];
      if(source.active)
      {
         Str src_proj_path=cei().sourceProjPath(source.loc.id); if(!src_proj_path.is() && source.loc==Str(AutoSource))src_proj_path=AutoSource;
         BuildFile &bf=build_files.New().set(BuildFile::SOURCE, src_proj_path, source.loc);
         if(!bf.src_proj_path.is()){ok=false; msgs.New().error(S+"Unknown project path for file \""+source.loc.asText()+"\".");}

         // count the number of braces
         if(!source.header && !source.cpp)
         {
            Int level=0;
            FREPA(source.tokens)switch((*source.tokens[i])[0])
            {
               case '{':     level++;                                                                                             break;
               case '}': if(!level--){ok=false; msgs.New().error("Invalid closing brace", source.tokens[i]); goto found_invalid;} break;
            }
            if(level){ok=false; msgs.New().error("The number or opening braces '{' does not match the number of closing braces '}'", &source, -1);}
         found_invalid:;
         }
      }
      if(source.used() && source.modified())if(!source.overwrite()){ok=false; msgs.New().error(S+"Can't overwrite changes to file \""+source.loc.asText()+"\".");} // overwrite changes in sources
   }
   REPA(build_files)if(build_files[i].mode==BuildFile::TEXT)if(Source *s=findSource(build_files[i].src_loc))if(s->modified())if(!s->overwrite()){ok=false; msgs.New().error(S+"Can't overwrite changes to file \""+s->loc.asText()+"\".");} // overwrite changes in text files

   return ok;
}
/******************************************************************************/
static void CleanNameForLinux(Str &s)
{
   if(Equal(s, "all"  , true))s="All"  ;else // compiling app named "all"   will result in endless loop on Linux
   if(Equal(s, "build", true))s="Build";else // compiling app named "build" will result in endless loop on Linux
   if(Equal(s, "clean", true))s="Clean";else // compiling app named "clean" will result in endless loop on Linux
   if(Equal(s, "help" , true))s="Help" ;else // compiling app named "help"  will result in endless loop on Linux
   if(Equal(s, "test" , true))s="Test" ;     // compiling app named "test"  will result in endless loop on Linux
}
static Str CleanNameForMakefile(C Str &s)
{
   Str o; o.reserve(s.length());
   FREPA(s)
   {
      Char c=s[i];
      switch(c) // these characters fail even with escaping
      {
         case '(':
         case ')':
         case '\'':
         case '=':
         case '`':
         case ';': c='_'; break;
         case '&': c='n'; break;
      }
      o+=c;
   }
   CleanNameForLinux(o);
   return o;
}
static Str CleanNameForNetBeans(C Str &s)
{
   Str o; o.reserve(s.length());
   FREPA(s)
   {
      Char c=s[i];
      switch(c) // these characters are not accepted by NetBeans
      {
         case '!':
         case '@':
         case '#':
         case '%':
         case '^':
         case '(':
         case ')':
         case '<':
         case '>':
         case '[':
         case ']':
         case '=':
         case '+':
         case ',':
         case ';':
         case '"':
         case '\'':
         case '`':
         case '~':
         case ' ': c='_'; break;
         case '&': c='n'; break;
      }
      o+=c;
   }
   CleanNameForLinux(o);
   return o;
}
Bool CodeEditor::verifyBuildPath()
{
   if(!getBuildPath(build_path, build_project_name))return false;
   if(!FCreateDirs(build_path))return false;
   build_source=build_path+"Source\\"; FCreateDir(build_source);
   switch(build_exe_type)
   {
      case EXE_EXE  : build_exe=build_path+build_project_name+".exe"; break;
      case EXE_DLL  : build_exe=build_path+build_project_name+".dll"; break;
      case EXE_LIB  : build_exe=build_path+build_project_name+".lib"; break;
      case EXE_UWP  : build_exe=build_path+build_project_name+".exe"; break;
      case EXE_MAC  : build_exe=build_path+build_project_name+".app"; break;
      case EXE_IOS  : build_exe=build_path+build_project_name+".app"; break;
      case EXE_APK  : build_exe=build_path+"Android/bin/"+build_project_name; break;
      case EXE_LINUX: build_exe=build_path+CleanNameForMakefile(build_project_name); break;
      case EXE_WEB  : build_exe=build_path+"Emscripten/"+(build_debug ? "Debug DX11/" : "Release DX11/")+build_project_name+".html"; break; // warning: this must match codes below if(build_exe_type==EXE_WEB)config+=" DX11";
      case EXE_NS   : build_exe=build_path+"NX64/"      +(build_debug ? "Debug DX11/" : "Release DX11/")+build_project_name+".nsp" ; break; // warning: this must match codes below if(build_exe_type==EXE_NS )config+=" DX11";
      default       : build_exe.clear(); return false;
   }
   return true;
}
Bool CodeEditor::adjustBuildFiles()
{
   // fix duplicate base names, by converting them to "xxxUNIQUE_NAMEi", where 'xxx'=original base name, 'i'=index of a duplicate
   {
           build_files.sort(Compare); // in order for files to always have the same names, we need to process them in the same order, so sort them
      REPA(build_files)build_files[i].ext_not=GetExtNot(build_files[i].src_proj_path);
      REPA(build_files)
      {
         BuildFile &bf=build_files[i]; if(bf.includeInProj())
         {
            Int same=0;
            Str base=GetBase(bf.ext_not);
            REPD(j, i)
            {
               BuildFile &bf2=build_files[j];
               if(bf2.includeInProj())
                  if(GetBase(bf2.ext_not)==base)bf2.ext_not+=S+UNIQUE_NAME+(same++);
            }
         }
      }
   }

   // set target file names
   REPAO(build_files).adjustPaths(build_path);

   return true;
}
/******************************************************************************/
static Int CompareByParent(Symbol* C &a, Symbol* C &b)
{
   if(Int c=ComparePath(a->parent ? a->parent->full_name : S, b->parent ? b->parent->full_name : S))return c;
   if(a->modifiers & b->modifiers & Symbol::MODIF_NAMELESS)return Compare(a->nameless_index, b->nameless_index); // if both are nameless, then use the nameless_index
   return   CompareCS  (*a, *b);
}
struct CodeFile
{
   Str      dest;
   FileText f;
   LineMap  lm;

   void operator++(int)
   {
      f.endLine();
      lm++;
   }
   void operator+=(C Str &line)
   {
      f.putLine(line);
      lm++;
   }
   void operator+=(C Memc<CodeLine> &lines)
   {
      Write(f, lines);
      lm.add(SourceLoc(), lines);
   }
   void add(C SourceLoc &src, C Memc<CodeLine> &lines)
   {
      Write(f, lines);
      lm.add(src, lines);
   }

   CodeFile(C Str &dest) {T.dest=dest; f.writeMem(UTF_8);} // some compilers don't support UTF-16, also UTF-8 will use less space, and faster to write
  ~CodeFile()
   {
      if(dest.is())
         if(OverwriteOnChangeLoud(f, dest))
            if(LineMap *lm=CE.build_line_maps.get(dest))Swap(*lm, T.lm);
   }
};
/******************************************************************************/
void CodeEditor::generateHeadersH(Memc<Symbol*> &sorted_classes, EXPORT_MODE export_mode)
{
   Memc<CodeLine> lines;
   Memc<Symbol* > symbols;
   Symbol        *Namespace=null;

   CodeFile f(build_source+UNIQUE_NAME+UNIQUE_NAME+"headers.h");
   f+=SEP_LINE;
   if(1 // for unknown reason this is needed also on Linux, so for simplicity, just enable it always
   || export_mode==EXPORT_ANDROID){f+="#pragma once"; f++;} // on Android all CPP files are packed into one for faster compilation, so we must make sure that this header can work if included multiple times

   // data types
   f+="#define bool Bool // boolean value (8-bit)";
   f++;
   f+="#define char8 Char8 //  8-bit character";
   f+="#define char  Char  // 16-bit character";
   f++;
   f+="#define sbyte  I8  //  8-bit   signed integer";
   f+="#define  byte  U8  //  8-bit unsigned integer";
   f+="#define  short I16 // 16-bit   signed integer";
   f+="#define ushort U16 // 16-bit unsigned integer";
   f+="#define  int   I32 // 32-bit   signed integer";
   f+="#define uint   U32 // 32-bit unsigned integer";
   f+="#define  long  I64 // 64-bit   signed integer";
   f+="#define ulong  U64 // 64-bit unsigned integer";
   f++;
   f+="#define flt Flt // 32-bit floating point";
   f+="#define dbl Dbl // 64-bit floating point";
   f++;
   f+="#define  ptr  Ptr // universal pointer";
   f+="#define cptr CPtr // universal pointer to const data";
   f++;
   f+="#define cchar8 CChar8 // const Char8";
   f+="#define cchar  CChar  // const Char16";
   f++;
   f+="#define  intptr  IntPtr //   signed integer capable of storing full memory address";
   f+="#define uintptr UIntPtr // unsigned integer capable of storing full memory address";
   f++;
   f+="#define class struct // Esenthel Script \"class\" is a C++ \"struct\"";

   // forward headers
   f+=SEP_LINE;

   // custom macros
   f+="// DEFINES";
   f+=S+"#define STEAM   "+cei().appPublishSteamDll ();
   f+=S+"#define OPEN_VR "+cei().appPublishOpenVRDll();
   f+=SEP_LINE;

   // write class forward declarations
   {
      Memc<Symbol*> &classes=symbols; classes.clear();
      FREPA(Symbols)
      {
         Symbol &symbol   =Symbols.lockedData(i);
         Bool    processed=false;
         if( symbol.type==Symbol::KEYWORD           )processed=true; // keywords are already processed
         if(!symbol.source || !symbol.source->active)processed=true; // symbols which aren't in current build files (for example EE headers) set as processed
         FlagSet(symbol.helper, Symbol::HELPER_PROCESSED|Symbol::HELPER_PROCESSED_FULL, processed);

         if(!processed)
            if(symbol.type==Symbol::CLASS && symbol.valid && symbol.isGlobal() && !(symbol.modifiers&Symbol::MODIF_NAMELESS)) // process only global named classes
               classes.add(&symbol);
      }
      classes.sort(CompareByParent);
      FREPA(classes)
      {
         Symbol    &Class =*classes[i]; Class.helper|=Symbol::HELPER_PROCESSED;
         Source    &source=*Class.source;
         Int        start = source.getSymbolStart(Class.token_index);
         if(InRange(start, source.tokens))
         {
            AdjustNameSymbol(lines, Namespace, Class.Namespace());
            Int line_start=lines.elms(); source.write(lines, start, Class.token_index); if(lines.elms())lines.last().append(';', TOKEN_OPERATOR);

            // expand templates
                                 source.expandTemplate(lines,              start , line_start); // only 'start' token needs to be checked
            REPA(Class.templates)source.expandTypename(lines, *Class.templates[i], line_start);
         }
      }
      if(lines.elms())
      {
         AdjustNameSymbol(lines, Namespace, null);
         f+=SEP_LINE;
         f+="// CLASSES";
         f+=SEP_LINE;
         f+=lines; lines.clear();
      }
   }

   // TODO: write caches (this is needed so cache typedefs 'CacheElmPtr' will work, more complex solution would be to make a better symbol ordering / dependency checker)
   FREPA(Symbols)
   {
      Symbol &symbol=Symbols.lockedData(i);
      if(symbol.isVar() && symbol.valid && symbol.isGlobal() && symbol.source && symbol.source->isFirstVar(symbol) && !(symbol.helper&Symbol::HELPER_PROCESSED) && symbol.value && Equal(symbol.value->full_name, "EE\\Cache", true))
      {
         symbol.helper|=Symbol::HELPER_PROCESSED;

         // adjust namespaces to typedef namespace
         AdjustNameSymbol(lines, Namespace, symbol.Namespace());

         symbol.source->writeSymbolDecl(lines, symbol);
         symbol.source->removeDefVal   (lines, symbol);
      }
   }
   if(lines.elms())
   {
      AdjustNameSymbol(lines, Namespace, null);
      f+=SEP_LINE;
      f+="// CACHES";
      f+=SEP_LINE;
      f+=lines; lines.clear();
   }

   // enums
   {
      Memc<Symbol*> namespaces;
      Memc<Symbol*> &enums=symbols; enums.clear();
      FREPA(Symbols)
      {
         Symbol &symbol=Symbols.lockedData(i);
         if(symbol.type==Symbol::ENUM && symbol.valid && symbol.isGlobal()) // global enums
            if(Source *source=symbol.source)if(source->active) // from active sources only
               enums.add(&symbol);
      }
      enums.sort(CompareByParent);

      f+=SEP_LINE;
      f+="// ENUMS";
      f+=SEP_LINE;
      FREPA(enums)
      {
         Symbol &symbol=*enums[i];
         Source &source=*symbol.source;
         symbol.helper|=Symbol::HELPER_PROCESSED|Symbol::HELPER_PROCESSED_FULL;
         Int start=source.getSymbolStart(symbol.token_index),
             end  =source.getBodyEnd    (start);
         if(InRange(start, source.tokens))
         {
            namespaces.clear(); for(Symbol *name=symbol.Parent(); name && name->type==Symbol::NAMESPACE; name=name->Parent())namespaces.New()=name;

            // start namespaces
            REPA(namespaces)lines.New().append("namespace", TOKEN_KEYWORD).append(' ', TOKEN_NONE).append(*namespaces[i], TOKEN_CODE).append('{', TOKEN_OPERATOR);
            if(namespaces.elms())lines.New().append(SEP_LINE, TOKEN_COMMENT);

            // write enum
            source.write(lines, start, end);

            // insert ';' at the end of struct/class/enum declarations
            // convert '.' to "->" or "::" when needed
            for(Int i=start; i<=end; i++)
            {
               Token &c=*source.tokens[i]; source.adjustToken(lines, i);
               if(c=='}' && c.parent)if(c.parent->type==Symbol::CLASS || c.parent->type==Symbol::ENUM) // put ';'
                  if(!InRange(i+1, source.tokens) || (*source.tokens[i+1])!=';') // only if it's not already there (this caused issues when the struct/class/enum was defined by a macro, like UNION)
               {
                  Int col; if(CodeLine *cl=FindLineCol(lines, c.pos(), col))cl->insert(col+1, ';', TOKEN_OPERATOR);
               }
            }

            // clean 'TOKEN_REMOVE'
            Clean(lines);

            // remove starting spaces
            REP(source.tokens[start]->col)
            {
               REPA (lines)if(lines[i].cols.elms() && ValidType(lines[i].cols[0].type))goto finished_enum_start_spaces;
               REPAO(lines).remove(0);
            }
         finished_enum_start_spaces:;

            // close namespaces
            lines.New().append(SEP_LINE, TOKEN_COMMENT);
            REPA(namespaces)lines.New().append('}', TOKEN_OPERATOR).append(' ', TOKEN_NONE).append(S+"// namespace "+*namespaces[i], TOKEN_COMMENT);
            if(namespaces.elms())lines.New().append(SEP_LINE, TOKEN_COMMENT);

            // write
            f+=lines; lines.clear();
         }
      }
   }

   // constants
   {
      Symbol *Namespace=null;
      FREPA(Symbols)
      {
            Symbol &symbol=Symbols.lockedData(i);
         if(Source *source=symbol.source)
            if(source->active    // only symbols from current build files
            && symbol.valid      // only valid symbols
            && symbol.isGlobal() // only global vars
            && symbol.isVar() && source->isFirstVar(symbol) && symbol.constDefineInHeader()) // process only first variables on the list
         {
            Int start=source->getSymbolStart(symbol.token_index),
                end  =source->getListEnd    (symbol.token_index);

            // adjust namespaces to variable namespace
            AdjustNameSymbol(lines, Namespace, symbol.Namespace());

            // write full variable list until first ';' encountered
            Int var_start=lines.elms();
            source->write(lines, start, end);
            // TODO: make more nicer to look

            // adjust token
            for(Int i=start; i<=end; i++)source->adjustToken(lines, i);
         }
      }

      // adjust namespaces to global namespace
      AdjustNameSymbol(lines, Namespace, null);

      // clean & parse
      Clean(lines);
      Parse(lines);

      // write
      if(lines.elms())
      {
         f+="// CONSTANTS";
         f+=SEP_LINE;
         f+=lines; lines.clear();
      }
   }

   // write typedefs
   {
      Memc<Symbol*> &typedefs=symbols; typedefs.clear();
      FREPA(Symbols)
      {
         Symbol &symbol=Symbols.lockedData(i);

         if(!(symbol.helper&Symbol::HELPER_PROCESSED))
            if(symbol.type==Symbol::TYPEDEF && symbol.source && symbol.valid && symbol.value && symbol.value->valid && symbol.isGlobal()) // process only global typedefs
               typedefs.add(&symbol);
      }
      for(Memc<Symbol*> to_process; ; )
      {
         REPA(typedefs)
         {
            Symbol &Typedef=*typedefs[i];
            if(Typedef.value->helper&Symbol::HELPER_PROCESSED){to_process.add(&Typedef); typedefs.remove(i);} // write the typedefs only if their target was already processed (this skips nameless classes which were not processed)
         }
         if(to_process.elms())
         {
            to_process.sort(CompareByParent);
            FREPA(to_process)
            {
               Symbol &Typedef=*to_process[i]; Typedef.helper|=Symbol::HELPER_PROCESSED|Symbol::HELPER_PROCESSED_FULL;
               Source &source =*Typedef.source;
               if(     source.isFirstVar(Typedef)) // process only first typedef
               {
                  // adjust namespaces to typedef namespace
                  AdjustNameSymbol(lines, Namespace, Typedef.Namespace());

                  source.writeSymbolDecl(lines, Typedef);
               }
            }
            to_process.clear();
         }else break;
      }
      if(lines.elms())
      {
         AdjustNameSymbol(lines, Namespace, null);
         f+=SEP_LINE;
         f+="// TYPEDEFS";
         f+=SEP_LINE;
         f+=lines; lines.clear();
      }
   }

   f+=SEP_LINE;

   // classes
   {
      f+=SEP_LINE;
      f+="// CLASSES";
      f+=SEP_LINE;

      FREPA(sorted_classes)
         if(Source *source=sorted_classes[i]->source)
      {
         if(source->writeClass(lines, *sorted_classes[i]))
         {
         #if MERGE_HEADERS
            f.add(source->loc, lines);
         #else
            Str rel=S+UNIQUE_NAME+sorted_classes[i]->fileName()+".h";
            CodeFile f2(build_source+rel); f2.add(source->loc, lines); f+=S+"#include \""+UnixPath(rel)+"\""; // Unix compilers don't support '\' paths
         #endif
         }
         lines.clear();
      }
   }

   // cpp headers
   f+=SEP_LINE;
   f+="// CPP";
   f+=SEP_LINE;
   FREPA(build_files)
   {
      BuildFile &bf=build_files[i];
      if(bf.mode==BuildFile::SOURCE)
         if(Source *source=findSource(bf.src_loc))
      {
         if(source->writeVarFuncs(lines))
         {
         #if MERGE_HEADERS
            f.add(source->loc, lines);
         #else
            Str full=GetExtNot(bf.dest_file_path)+".h";
            CodeFile f2(full); f2.add(source->loc, lines); f+=S+"#include \""+UnixPath(SkipStartPath(full, build_source))+"\""; // Unix compilers don't support '\' paths
         #endif
         }
         lines.clear();
      }
   }

   // inline/template classes and functions
   f+=SEP_LINE;
   f+="// INLINE, TEMPLATES";
   f+=SEP_LINE;
   FREPA(build_files)
   {
      BuildFile &bf=build_files[i];
      if(bf.mode==BuildFile::SOURCE)
         if(Source *source=findSource(bf.src_loc))
      {
         if(source->writeInline(lines))
         {
         #if MERGE_HEADERS
            f.add(source->loc, lines);
         #else
            Str full=GetExtNot(bf.dest_file_path)+".inline.h";
            CodeFile f2(full); f2.add(source->loc, lines); f+=S+"#include \""+UnixPath(SkipStartPath(full, build_source))+"\""; // Unix compilers don't support '\' paths
         #endif
         }
         lines.clear();
      }
   }

   f+=SEP_LINE;
}
/******************************************************************************/
static Int CompareBySource(Symbol* C &a, Symbol* C &b)
{
   if(Int c=SourceLoc::Compare(a->source->loc, b->source->loc))return c;
   return              Compare(a->token_index, b->token_index);
}
static void ListHeaders(FileText &ft, Memc<Str> &headers, C Str &condition)
{
   if(headers.elms())
   {
      ft.putLine(S+"#if "+condition);
      ft.depth++;
      FREPA(headers)ft.putLine(S+"#include \""+headers[i]+'"'); // use "header" instead of <header> because Mac fails to compile user headers with <> but succeeds both user+system with ""
      ft.depth--;
      ft.putLine("#endif");
   }
}
Bool CodeEditor::generateCPPH(Memc<Symbol*> &sorted_classes, EXPORT_MODE export_mode)
{
   // setup nameless indexes needed for file names
   Memc<Symbol*> nameless;
   FREPA(Symbols){Symbol &symbol=Symbols.lockedData(i); if((symbol.modifiers&Symbol::MODIF_NAMELESS) && symbol.source && symbol.source->active && symbol.valid && symbol.isGlobal())nameless.add(&symbol);}
   nameless.sort(CompareBySource); // sort by source, so that we can have constant 'nameless_index' for the same symbols (so when sources are reloaded, the same symbols, have the same indexes as before)
   REPAO(nameless)->nameless_index=i;

   // detect default ctors
   FREPA(build_files)
   {
      BuildFile &bf=build_files[i];
      if(bf.mode==BuildFile::SOURCE)
         if(Source *source=findSource(bf.src_loc))
            source->detectDefaultCtors(); // this probably requires to be processed per-source
   }

   // count number of source files
   Int  sources=0; REPA(build_files)if(build_files[i].mode==BuildFile::SOURCE)sources++;
   Bool build_headers_in_cpp=(sources<=8);

   // make cpp files
   FREPA(build_files)
   {
      BuildFile &bf=build_files[i];
      if(bf.mode==BuildFile::SOURCE)
      {
         /*FileInfo src, dest; if(src.get(bf.)) can't use this because 'build_headers_in_cpp' sometimes generates different CPP files
         {
            if(dest.get(bf.dest_file_path))
               if(!Compare(src.modify_time_utc, dest.modify_time_utc, 1))continue; // if 'src' date is the same as 'dest' date then assume they're the same
         }*/
         if(Source *source=findSource(bf.src_loc))
            source->makeCPP(build_source, bf.dest_file_path, build_headers_in_cpp);
      }
   }

   // make headers
   generateHeadersH(sorted_classes, export_mode);

   // generate precompiled header
   {
      FileText ft;

      // stdafx.cpp
      ft.write(build_path+"stdafx.cpp", ANSI); ft.putLine("#include \"stdafx.h\""); ft.del(); DateTime dt; dt.zero(); dt.year=2010; dt.month=1; dt.day=1; FTimeUTC(build_path+"stdafx.cpp", dt);

      // stdafx.h
         ft.writeMem();
         ft.putLine("#pragma once"); // on Apple this header gets included automatically because of project PCH settings, and because of manual "#include "stdafx.h"" needed on Windows, it would get included multiple times

         // 3rd party headers (first)
         Memc<Str> win_headers=GetFiles(cei().appHeadersWindows ()),
                   mac_headers=GetFiles(cei().appHeadersMac     ()),
                 linux_headers=GetFiles(cei().appHeadersLinux   ()),
               android_headers=GetFiles(cei().appHeadersAndroid ()),
                   ios_headers=GetFiles(cei().appHeadersiOS     ()),
              nintendo_headers=GetFiles(cei().appHeadersNintendo());
       /*REPA(headers)
         {
            Str &header=headers[i]; if(Ends(header, "?optional"))
            {
               header.removeLast(9); if(!FExistSystem(header))headers.remove(i, true);
            }
         }*/
         Str bin_path=BinPath();
         if(win_headers.elms() || mac_headers.elms() || linux_headers.elms() || android_headers.elms() || ios_headers.elms())
         {
            ft.putLine(S+"#include \""+UnixPath(bin_path+"EsenthelEngine\\_\\System\\begin.h")+'"');
            ListHeaders(ft, win_headers, "defined _WIN32");
            if(mac_headers.elms() || ios_headers.elms())
            {
               ft.putLine("#ifdef __APPLE__");
               ft.depth++;
               ft.putLine("#include <TargetConditionals.h>");
               ListHeaders(ft, mac_headers, "!TARGET_OS_IPHONE");
               ListHeaders(ft, ios_headers,  "TARGET_OS_IPHONE");
               ft.depth--;
               ft.putLine("#endif");
            }
            ListHeaders(ft,    linux_headers, "defined __linux__ && !defined ANDROID // Android also has '__linux__' defined");
            ListHeaders(ft,  android_headers, "defined ANDROID");
            ListHeaders(ft, nintendo_headers, "defined __NINTENDO__");
            ft.putLine(S+"#include \""+UnixPath(bin_path+"EsenthelEngine\\_\\System\\end.h")+'"');
         }

         // EE headers (second)
         ft.putLine(S+"#include \""+UnixPath(bin_path+"EsenthelEngine\\EsenthelEngine.h")+'"');

         if(!build_headers_in_cpp)ft.putLine(UnixPath(S+"#include \"Source\\"+UNIQUE_NAME+UNIQUE_NAME+"headers.h\""));
         OverwriteOnChangeLoud(ft, build_path+"stdafx.h");
   }

   return true;
}
/******************************************************************************/
struct BuildFileElm
{
   struct Name
   {
      Bool file;
      Str  name;

      Name(Bool file, C Str &name) {T.file=file; T.name=name;}
   };

   static Int Compare(C Node<BuildFileElm> &elm, C Name &name)
   {
      if(Int c=::Compare(elm.file()   , name.file))return c; // folders first
      return   CompareCI(elm.base_name, name.name);
   }

   Str                    base_name, full_name;
   CodeEditor::BuildFile *bf;
   U64                    id;

   Bool file()C {return bf!=null;} // if elm is file or folder

   void set(C Str &base, C Str &full) {T.base_name=base; T.full_name=full;}

   BuildFileElm() {bf=null; id=0;}
};
static void BuildTree(Node<BuildFileElm> &node, CodeEditor::BuildFile &bf, Str path)
{
   if(bf.includeInProj())
   {
      Str start=GetStart(path); path=GetStartNot(path);
      Int index; if(!node.children.binarySearch(BuildFileElm::Name(!path.is(), start), index, BuildFileElm::Compare))node.children.NewAt(index).set(start, Str(node.full_name).tailSlash(true)+start); // keep sorted
      Node<BuildFileElm> &elm=node.children[index];
      if(path.is())BuildTree(elm, bf, path);else elm.bf=&bf;
   }
}
static void StoreTree9(Node<BuildFileElm> &bfe, XmlNode &node)
{
   FREPA(bfe.children)
   {
      Node<BuildFileElm> &elm=bfe.children[i];
      if(elm.bf) // file
      {
         XmlNode &file=node.nodes.New().setName("File");
         if(elm.bf->mode==CodeEditor::BuildFile::SOURCE)
         {
            file.params.New().set("RelativePath", S+".\\"+elm.bf->dest_proj_path);
         }else
         {
            file.params.New().set("RelativePath", elm.bf->dest_proj_path);
         }
      }else // folder
      {
         XmlNode &filter=node.nodes.New().setName("Filter");
         filter.params.New().set("Name", elm.base_name);
         StoreTree9(elm, filter);
      }
   }
}
static void StoreTree10(Node<BuildFileElm> &bfe, XmlNode &node)
{
   FREPA(bfe.children)
   {
      Node<BuildFileElm> &elm=bfe.children[i];
      if(elm.bf) // file
      {
         XmlNode &file  =node.nodes.New().setName((elm.bf->mode==CodeEditor::BuildFile::SOURCE) ? "ClCompile" : (elm.bf->mode==CodeEditor::BuildFile::LIB) ? "Library" : (elm.bf->mode==CodeEditor::BuildFile::COPY) ? "CopyFileToFolders" : "None"); file.params.New().set("Include", elm.bf->dest_proj_path);
         XmlNode &filter=file.nodes.New().setName("Filter"); filter.data.New()=GetPath(elm.full_name);
      }else // folder
      {
         XmlNode &filter=node.nodes.New().setName("Filter");
         filter.params.New().set("Include", elm.full_name);
         StoreTree10(elm, node);
      }
   }
}
/******************************************************************************/
static void SetFile(FileText &f, C Str &s, ENCODING encoding=UTF_8)
{
   f.writeMem(encoding).putText(s);
}
/******************************************************************************/
static Bool SafeOverwriteLoud(File &f, C Str &dest, C DateTime *modify_time_utc=null)
{
   if(!SafeOverwrite(f, dest, modify_time_utc))return ErrorWrite(dest);
   return true;
}
static Bool CopyFile(C Str &src, C Str &dest)
{
   if(!FCopy(src, dest, FILE_OVERWRITE_DIFFERENT))return ErrorCopy(src, dest);
   return true;
}
static Bool CopyFile(C Str &src, C Str &dest, Bool &changed)
{
   FileInfoSystem fi(dest);            Bool   ret=CopyFile(src, dest);
   changed=(fi!=FileInfoSystem(dest)); return ret;
}
/******************************************************************************/
static void Add(Memb<PakFileData> &files, Pak &pak, C PakFile &pf)
{
   PakFileData &pfd=files.New();
   pfd.name             =pak.fullName(pf);
   pfd.xxHash64_32      =pf.data_xxHash64_32;
   pfd.modify_time_utc  =pf.modify_time_utc;
   pfd.type             =pf.type();
   pfd.compressed       =pf.compression;
   pfd.decompressed_size=pf.data_size;
   if(pf.data_size)pfd.data.set(pf, pak);
}
static void AddFile(Memb<PakFileData> &files, Pak &pak, C PakFile &pf)
{
   if(!FlagTest(pf.flag, PF_STD_DIR))Add(files, pak, pf); // skip folders
}
static void Add        (Memb<PakFileData> &files,           C PaksFile *pf) {if(pf)Add    (files, *pf->pak, *pf->file);}
static void AddFile    (Memb<PakFileData> &files, Pak &pak, C PakFile  *pf) {if(pf)AddFile(files,      pak, *pf      );}
static void AddFile    (Memb<PakFileData> &files,           C PaksFile *pf) {if(pf)AddFile(files, *pf->pak, *pf->file);}
static void Add        (Memb<PakFileData> &files, Pak &pak, C PakFile  *pf) {if(pf)Add    (files,      pak, *pf      );}
static void AddChildren(Memb<PakFileData> &files,           C PaksFile *pf) {if(pf)FREP(pf->children_num)AddFile(files,     &Paks.file(pf->children_offset+i));}
static void AddChildren(Memb<PakFileData> &files, Pak &pak, C PakFile  *pf) {if(pf)FREP(pf->children_num)AddFile(files, pak, pak .file(pf->children_offset+i));}
static Bool CreateEngineEmbedPak(C Str &src, C Str &dest, Bool use_cipher, Bool *changed=null)
{
   if(changed)*changed=false;

   Memb<PakFileData> files;
   Pak temp, *src_pak=null;
   if(src.is()) // use specific PAK file
   {
      if(!temp.load(src))return ErrorRead(src); src_pak=&temp;
   }else // if empty then find from loaded PAK's in memory
   {
      SyncLocker locker(Paks._lock);
      FREPA(Paks._paks) // iterate all loaded paks starting from those loaded first
      {
         PakSet::Src &src=Paks._paks[i];
         if(GetBase(src.name)=="Engine.pak")
         {
            src_pak=&src;
            break;
         }
      }
   }
   if(!src_pak)return Error("Can't find \"Engine.pak\"");

   if(CE.cei().appEmbedEngineData()>1) // full
   {
      FREPA(*src_pak)Add(files, *src_pak, src_pak->file(i)); // add all files
   }else // 2D only
   {
      // add folders to preserve modification times
      Add    (files, *src_pak, src_pak->find("Shader"));
      Add    (files, *src_pak, src_pak->find("Shader/4"));
      Add    (files, *src_pak, src_pak->find("Shader/GL"));
      Add    (files, *src_pak, src_pak->find("Shader/GL SPIR-V"));
      Add    (files, *src_pak, src_pak->find("Gui"));
      AddFile(files, *src_pak, src_pak->find("Shader/4/Early Z"));
      AddFile(files, *src_pak, src_pak->find("Shader/4/Main"));
      AddFile(files, *src_pak, src_pak->find("Shader/4/Position"));
      AddFile(files, *src_pak, src_pak->find("Shader/GL/Early Z"));
      AddFile(files, *src_pak, src_pak->find("Shader/GL/Main"));
      AddFile(files, *src_pak, src_pak->find("Shader/GL/Position"));
      AddFile(files, *src_pak, src_pak->find("Shader/GL SPIR-V/Early Z"));
      AddFile(files, *src_pak, src_pak->find("Shader/GL SPIR-V/Main"));
      AddFile(files, *src_pak, src_pak->find("Shader/GL SPIR-V/Position"));
      
      AddChildren(files, *src_pak, src_pak->find("Gui"));
      FREP(src_pak->rootFiles())AddFile(files, *src_pak, src_pak->file(i)); // add all root files (gui files)
   }


   Cipher *cipher=(use_cipher ? CE.cei().appEmbedCipher() : null);
   if(CompareFile(FileInfoSystem(dest).modify_time_utc, CE.cei().appEmbedSettingsTime())>0 && PakEqual(files, dest, cipher))return true; // if existing Pak time is newer than settings (compression/encryption) and Pak is what we want, then use it
   if(changed)*changed=true;
   if(!PakCreate(files, dest, 0, cipher, CE.cei().appEmbedCompress(), CE.cei().appEmbedCompressLevel()))return Error("Can't create Embedded Engine Pak");
   return true;
}
static Bool CreateAppPak(C Str &name, Bool &exists, Bool *changed=null)
{
   Bool ok=false;
   exists=false;
   if(changed)*changed=false;

   Memb<PakFileData> files; CE.cei().appSpecificFiles(files);
   if(files.elms())
   {
      exists=true;
      Cipher *cipher=CE.cei().appEmbedCipher();
      if(CompareFile(FileInfoSystem(name).modify_time_utc, CE.cei().appEmbedSettingsTime())>0 && PakEqual(files, name, cipher))ok=true; // if existing Pak time is newer than settings (compression/encryption) and Pak is what we want, then use it
      else {if(changed)*changed=true; ok=PakCreate(files, name, 0, cipher, CE.cei().appEmbedCompress(), CE.cei().appEmbedCompressLevel());}
   }else
   {
      ok=true;
      Bool c=FDelFile(name); if(changed)*changed=c;
   }
   if(!ok)Error("Can't create app pak");
   return ok;
}
static void DelExcept(C Str &path, CChar8 *allowed[], Int allowed_elms)
{
   for(FileFind ff(path); ff(); )
   {
      REP(allowed_elms)if(ff.name==allowed[i])goto keep;
      FDel(ff.pathName());
   keep:;
   }
}
static void Optimize(Image &image)
{
   Vec4 min; if(image.stats(&min))if(min.w>=1-1.5f/255)image.copyTry(image, -1, -1, -1, IMAGE_R8G8B8_SRGB); // if image has no alpha, then remove it, because it will reduce PNG size
   if(image.typeInfo().a) // if image has alpha, then zero pixels without alpha to further improve compression
      REPD(y, image.h())
      REPD(x, image.w())
   {
      Color c=image.color(x, y);
      if(!c.a)image.color(x, y, TRANSPARENT);
   }
}
static Bool GetIcon(Image &image, DateTime &modify_time_utc)
{
   image.del(); modify_time_utc.zero();
   if(C ImagePtr &app_icon=CE.cei().appIcon()){app_icon->copyTry(image, -1, -1, 1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1, FILTER_BEST, IC_CLAMP|IC_ALPHA_WEIGHT); modify_time_utc=FileInfo(app_icon.name()).modify_time_utc;}
   if(!image.is()){image.ImportTry("Code/Icon.ico", -1, IMAGE_SOFT, 1); modify_time_utc=FileInfo("Code/Icon.ico").modify_time_utc;}
   if( image.is())
   {
      if(!modify_time_utc.valid())modify_time_utc.getUTC();
      Optimize(image);
      return true;
   }
   return false;
}
static void GetImages(Image &portrait, DateTime &portrait_time, Image &landscape, DateTime &landscape_time)
{
    portrait_time.zero(); if(C ImagePtr &app_portrait =CE.cei().appImagePortrait ()){app_portrait ->copyTry(portrait , -1, -1, -1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1, FILTER_BEST, IC_CLAMP|IC_ALPHA_WEIGHT);  portrait_time=FileInfo(app_portrait .name()).modify_time_utc; Optimize( portrait);} if(! portrait_time.valid()) portrait_time.getUTC();
   landscape_time.zero(); if(C ImagePtr &app_landscape=CE.cei().appImageLandscape()){app_landscape->copyTry(landscape, -1, -1, -1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1, FILTER_BEST, IC_CLAMP|IC_ALPHA_WEIGHT); landscape_time=FileInfo(app_landscape.name()).modify_time_utc; Optimize(landscape);} if(!landscape_time.valid())landscape_time.getUTC();
}
static void GetNotificationIcon(Image &image, DateTime &modify_time_utc, C Image &icon, DateTime &icon_time)
{
   if(C ImagePtr &app_icon=CE.cei().appNotificationIcon())
   {
      app_icon->mustCopy(image, -1, -1, -1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1, FILTER_BEST, IC_CLAMP|IC_ALPHA_WEIGHT); modify_time_utc=FileInfo(app_icon.name()).modify_time_utc; Optimize(image);
      if(!modify_time_utc.valid())modify_time_utc.getUTC();
   }else
   {
      icon.mustCopy(image, -1, -1, -1, IMAGE_L8A8_SRGB, IMAGE_SOFT, 1, FILTER_BEST, IC_CLAMP|IC_ALPHA_WEIGHT); // convert to grey
      modify_time_utc=icon_time;
   }
}
static Bool ImageResize(C Image &src, Image &dest, Int x, Int y, FIT_MODE fit)
{
   Bool ok=true;
   if(src.is())
   {
      VecI2 size_x=src.size()*x/src.w(), // uniformly scaled size to match target x
            size_y=src.size()*y/src.h(); // uniformly scaled size to match target y
      if((fit==FIT_FILL) ? size_x.y>=y  // if after scaling with size_x, height is more than enough
                         : size_x.y<=y) // if after scaling with size_x, height fits
      {
         Int image_type=((y>size_x.y && !src.typeInfo().a) ? ImageTypeIncludeAlpha(src.type()) : -1);
         ok=src.copyTry(dest, size_x.x, size_x.y, -1, image_type, IMAGE_SOFT, 1, FILTER_BEST, IC_CLAMP|IC_ALPHA_WEIGHT);
         Int d=size_x.y-y; dest.crop(dest, 0, d/2, dest.w(), y);
      }else
      {
         Int image_type=((x>size_y.x && !src.typeInfo().a) ? ImageTypeIncludeAlpha(src.type()) : -1);
         ok=src.copyTry(dest, size_y.x, size_y.y, -1, image_type, IMAGE_SOFT, 1, FILTER_BEST, IC_CLAMP|IC_ALPHA_WEIGHT);
         Int d=size_y.x-x; dest.crop(dest, d/2, 0, x, dest.h());
      }
   }
   return ok;
}
struct ImageConvert
{
   Bool          ok, _square, remove_alpha;
   FIT_MODE     fit;
   Byte      format; // 0=PNG, 1=ICO, 2=ICNS, 3=BMP
   Str         dest;
   VecI2  size,
         _crop,
         _clamp;
 C Image       *src;
   DateTime      dt;

   static void Func(ImageConvert &ic, Ptr user, Int thread_index) {ic.process();}

   ImageConvert& set(C Str &dest, C Image &src, C DateTime &dt)
   {
      T.ok=false; T._square=false; T.remove_alpha=false; T.format=0; T.dest=dest; T.size=T._crop=T._clamp=-1; T.fit=FIT_FULL; T.src=&src; T.dt=dt; return T;
   }
   ImageConvert& ICO        (            ) {T.format=1; return T;}
   ImageConvert& ICNS       (            ) {T.format=2; return T;}
   ImageConvert& BMP        (            ) {T.format=3; return T;}
   ImageConvert& square     (            ) {_square=true; return T;}
   ImageConvert& resize     (Int w, Int h) {T.size.set(w, h); return T;}
   ImageConvert& resizeFill (Int w, Int h) {T.size.set(w, h); T.fit=FIT_FILL; return T;}
   ImageConvert& crop       (Int w, Int h) { _crop.set(w, h); return T;}
   ImageConvert& clamp      (Int w, Int h) {_clamp.set(w, h); return T;}
   ImageConvert& removeAlpha(            ) {remove_alpha=true; return T;}

   void process()
   {
      File  f; f.writeMem();
      Image temp;
      if(size.x>0 || size.y>0)if(ImageResize(*src, temp, size.x, size.y, fit))src=&temp;else return;
      if(_clamp.x>0 || _clamp.y>0)
      {
         VecI2 size=src->size();
         if(_clamp.x>0 && size.x>_clamp.x)size.set(_clamp.x, Max(1, DivRound(size.y*_clamp.x, size.x)));
         if(_clamp.y>0 && size.y>_clamp.y)size.set(Max(1, DivRound(size.x*_clamp.y, size.y)), _clamp.y);
         if(src->copyTry(temp, size.x, size.y, -1, -1, IMAGE_SOFT, 1, FILTER_BEST, IC_CLAMP|IC_ALPHA_WEIGHT))src=&temp;else return;
      }
      if(_crop.x>0 && _crop.y>0)
      {
         if(!src->typeInfo().a && (_crop.x>src->w() || _crop.y>src->h()))if(src->copyTry(temp, -1, -1, -1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1))src=&temp;else return; // if we're cropping to a bigger size, then make sure that alpha channel is present, so pixels can be set to transparent color
         src->crop(temp, (src->w()-_crop.x)/2, (src->h()-_crop.y)/2, _crop.x, _crop.y); src=&temp;
      }
      if(_square && src->size().allDifferent())
      {
         if(!src->typeInfo().a)if(src->copyTry(temp, -1, -1, -1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1))src=&temp;else return; // if we're cropping to a bigger size, then make sure that alpha channel is present, so pixels can be set to transparent color
         Int size=src->size().max(); src->crop(temp, (src->w()-size)/2, (src->h()-size)/2, size, size); src=&temp;
      }
      if(remove_alpha && src->typeInfo().a)
      {
         Image temp1; if(temp1.createSoftTry(src->w(), src->h(), 1, IMAGE_R8G8B8_SRGB))
         {
            REPD(y, src->h())
            REPD(x, src->w())
            {
               Vec4 c=src->colorF(x, y); c.xyz*=c.w;
                     temp1.colorF(x, y, c);
            }
            Swap(temp, temp1); src=&temp;
         }
      }
      switch(format)
      {
         case 0: ok=src->ExportPNG (f, 1); break;
         case 1: ok=src->ExportICO (f   ); break;
         case 2: ok=src->ExportICNS(f   ); break;
         case 3: ok=src->ExportBMP (f   ); break;
      }
      if(ok){f.pos(0); FCreateDir(GetPath(dest)); ok=SafeOverwrite(f, dest, &dt);}
   }
};
Bool CodeEditor::generateVSProj(Int version)
{
   // !! For UWP don't store assets deeper than "Assets" (for example "Assets/UWP") because that would affect the final asset locations in the UWP executable !!

   if(build_exe_type!=EXE_EXE && build_exe_type!=EXE_DLL /*&& build_exe_type!=EXE_LIB*/ && build_exe_type!=EXE_UWP && build_exe_type!=EXE_WEB && build_exe_type!=EXE_NS)return Error("Visual Studio projects support only EXE, DLL, Universal, Web and NintendoSwitch configurations.");
   if(build_exe_type==EXE_WEB && version<10)return Error("WEB configuration requires Visual Studio 2010 or newer.");

   Str bin_path    =BinPath(false),
       bin_path_rel=BinPath();

   FCreateDirs(build_path+"Assets/Nintendo Switch");

   FileText resource_rc; resource_rc.writeMem(UTF_16); // utf-16 must be used, because VS has problems with utf-8
   FileText src; if(!src.read("Code/Windows/resource.rc"))return ErrorRead("Code/Windows/resource.rc"); for(; !src.end(); )resource_rc.putLine(src.fullLine());
   Bool     resource_changed=false;

   // Steam DLL
   if(cei().appPublishSteamDll()) // this must be copied always because the DLL needs to be present in the EXE folder, since we can't specify a custom path for it
   {
      CChar8 *name=(config_32_bit ? "steam_api.dll" : "steam_api64.dll");
      if(!CopyFile(S+"Code/Windows/"+name, build_path+name))return false;
   }

   // OpenVR DLL
   if(cei().appPublishOpenVRDll()) // this must be copied always because the DLL needs to be present in the EXE folder, since we can't specify a custom path for it
   {
      CChar8 *name=(config_32_bit ? "openvr_api.32.dll" : "openvr_api.64.dll");
      if(!CopyFile(S+"Code/Windows/"+name, build_path+"openvr_api.dll"))return false;
   }

   // d3dcompiler_47.dll (Windows 7 and earlier don't have it included, this is needed if app uses shader compilation, or if compiled in debug mode which does not remove unreferenced symbols/functions)
   {
      CChar8 *name=(config_32_bit ? "d3dcompiler_47.32.dll" : "d3dcompiler_47.64.dll");
      if(!CopyFile(S+"Code/Windows/"+name, build_path+"d3dcompiler_47.dll"))return false;
   }

   // file version !! must be first !! because VERSIONINFO is required to have ID=1, https://docs.microsoft.com/en-us/windows/desktop/menurc/versioninfo-resource "versionID = Version-information resource identifier. This value must be 1."
   Int resource_id=1; // ID of the first resource
   resource_rc.putLine(S+(resource_id++)+" VERSIONINFO FILEVERSION "+cei().appBuild()+",0,0,0 {}");

   // icon, generate always because it may be used by WINDOWS_OLD
   resource_rc.putLine(S+(resource_id++)+" ICON        \"Icon.ico\"");

   // embed engine data, generate always because it may be used by WINDOWS_OLD
   if(cei().appEmbedEngineData())
   {
      Bool changed; if(!CreateEngineEmbedPak(S, build_path+"Assets/EngineEmbed.pak", true, &changed))return false;
      resource_rc.putLine(S+(resource_id++)+" PAK         \"EngineEmbed.pak\"");
      resource_changed|=changed;
   }

   // app data, generate always because it may be used by WINDOWS_OLD
   {
      Bool exists, changed; if(!CreateAppPak(build_path+"Assets/App.pak", exists, &changed))return false;
      if(exists)resource_rc.putLine(S+(resource_id++)+" PAK         \"App.pak\"");
      resource_changed|=changed;
   }

   // generate images
   Image      icon; DateTime icon_time;
   if(GetIcon(icon,          icon_time))
   {
      Image portrait ; DateTime  portrait_time;
      Image landscape; DateTime landscape_time;
      GetImages(portrait, portrait_time, landscape, landscape_time);

      Memc<ImageConvert> convert;
      DateTime dt;
      CChar8 *rel;
      const Bool splash_from_icon=true;
      Image empty;

      rel="Assets/Icon.ico"; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time)){resource_changed=true; convert.New().set(build_path+rel, icon, icon_time).ICO().clamp(256, 256).square();} // Windows can't handle non-square icons properly (it stretches them)

      if(build_exe_type==EXE_UWP) // creating icons/images is slow, so do only when necessary
      {
         // list images starting from the smallest
         rel="Assets/Square44x44Logo.targetsize-48_altform-unplated.png"; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize( 48,  48); // this is used for Windows Taskbar       , for 1920x1080 screen, taskbar icon is around 32x32, no need to provide bigger size
         rel="Assets/Square44x44Logo.scale-200.png"                     ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize( 88,  88); // this is used for Windows Phone App List, for 1280x720  screen,         icon is  80x80
         rel="Assets/Square150x150Logo.scale-200.png"                   ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize(300, 300); // this is used for Windows Phone Start   , for 1280x720  screen,         icon is 228x228

         rel="Assets/Logo.png"; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize(50, 50);

         rel="Assets/SplashScreen.png";
         if(splash_from_icon)
         {
            if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize(128, 128).crop(620, 300);
         }else
         if(landscape.is() || portrait.is())
         {
            dt=(landscape.is() ? landscape_time : portrait_time); if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, dt))convert.New().set(build_path+rel, landscape.is() ? landscape : portrait, dt).resizeFill(620, 300);
         }else // use empty
         {
            dt.zero(); dt.day=1; dt.month=1; dt.year=2000; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, dt)){empty.createSoftTry(620, 300, 1, IMAGE_R8G8B8A8_SRGB); empty.zero(); convert.New().set(build_path+rel, empty, dt);}
         }
      }

      if(build_exe_type==EXE_NS) // creating icons/images is slow, so do only when necessary
      {
         rel="Assets/Nintendo Switch/Icon.bmp"; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize(1024, 1024).removeAlpha().BMP(); // NS accepts only 1024x1024 RGB (no alpha) BMP
      }

      convert.reverseOrder(); // start working from the biggest ones because they take the most time, yes this is correct
      MultiThreadedCall(convert, ImageConvert::Func);
      FREPA(convert)if(!convert[i].ok)return ErrorWrite(convert[i].dest);
   }

   // resources, generate always because it may be used by WINDOWS_OLD
   {
      Str path=build_path+"Assets/resource.rc";
      if(!OverwriteOnChangeLoud(resource_rc, path))return false;
      if(resource_changed)FTimeUTC(path, DateTime().getUTC()); // if any resource was changed then we need to adjust "resource.rc" modification time to make sure that VS will rebuild the resources
   }

   // xboxservices.config
   if(build_exe_type==EXE_EXE || build_exe_type==EXE_UWP)
   {
      ULong xbl_title_id=cei().appXboxLiveTitleID();
      UID   xbl_scid    =cei().appXboxLiveSCID   ();
      if(xbl_title_id || xbl_scid.valid()) // want to use Xbox
      {
         TextData td; TextNode &root=td.nodes.New();
         root.nodes.New().set("TitleId"               , xbl_title_id);
         root.nodes.New().set("PrimaryServiceConfigId", CaseDown(xbl_scid.asCanonical())); // CaseDown required
         root.nodes.New().set("XboxLiveCreatorsTitle" , TextBool(cei().appXboxLiveProgram()==XBOX_LIVE_CREATORS));
         FileText f; f.writeMem(); td.saveJSON(f); if(!OverwriteOnChangeLoud(f, build_path+"xboxservices.config"))return false;

         BuildFile &bf=build_files.New().set(BuildFile::COPY, S, SourceLoc());
         bf.dest_proj_path=    "xboxservices.config";
         bf.filter        ="H/_/xboxservices.config";
      }
   }

   // VS project
   Node<BuildFileElm> tree; FREPA(build_files)
   {
      BuildFile &bf=build_files[i];
      BuildTree(tree, bf, bf.filter.is() ? bf.filter : bf.dest_proj_path);
   }
   Memc<Str> libs_win=GetFiles(cei().appLibsWindows()),
             dirs_win=GetFiles(cei().appDirsWindows()),
             libs_web,
             dirs_web=dirs_win,
             libs_ns =GetFiles(cei().appLibsNintendo()),
             dirs_ns =dirs_win;

   // Web html
   if(build_exe_type==EXE_WEB)
   {
      if(!src.read("Code/Web/Page.html"))return ErrorRead("Code/Web/Page.html");
      Str page; src.getAll(page);
      page=Replace(page, "EE_PAGE_TITLE"  , XmlString   (build_project_name)      , true, WHOLE_WORD_STRICT);
      page=Replace(page, "EE_JS_FILE_NAME", GetBaseNoExt(build_exe         )+".js", true, WHOLE_WORD_STRICT);
      SetFile(src, page, UTF_8_NAKED);
      FCreateDirs(GetPath(build_exe));
      if(!OverwriteOnChangeLoud(src, GetExtNot(build_exe)+".Esenthel.html"))return false; // use custom suffix name because Emscripten uses GetExtNot(build_exe)+".html"
   }

   // manifest
   if(!CopyFile("Code/Windows/Windows Manifest.xml", build_path+"Assets/Windows Manifest.xml"))return false;

   // Nintendo Switch
   if(!CopyFile("Code/Nintendo Switch/ImportNintendoSdk.props", build_path+"ImportNintendoSdk.props"))return false;
   if(!CopyFile("Code/Nintendo Switch/Project.nnsdk.xml"      , build_path+"Project.nnsdk.xml"      ))return false;
   {
      XmlData xml;
      if(!xml.load("Code/Nintendo Switch/Project.nmeta"))return ErrorRead("Code/Nintendo Switch/Project.nmeta");
      if(XmlNode *NintendoSdkMeta=xml.findNode("NintendoSdkMeta"))
      {
         if(XmlNode *Core=NintendoSdkMeta->findNode("Core"))
         {
          //if(XmlNode *Name=Core->findNode("Name"))Name->data.setNum(1)[0]=cei().appName();
            if(auto app_id=cei().appNintendoAppID())if(XmlNode *ApplicationId=Core->findNode("ApplicationId"))ApplicationId->data.setNum(1)[0]=TextHex(app_id, 16, 0, true);
         }
         if(XmlNode *Application=NintendoSdkMeta->findNode("Application"))
         {
            if(XmlNode *Title=Application->findNode("Title"))
            {
               if(XmlNode *Name     =Title->findNode("Name"     ))Name     ->data.setNum(1)[0]=cei().appName();
               if(XmlNode *Publisher=Title->findNode("Publisher"))Publisher->data.setNum(1)[0]=cei().appNintendoPublisherName();
            }
            if(XmlNode *ReleaseVersion=Application->findNode("ReleaseVersion"))ReleaseVersion->data.setNum(1)[0]=cei().appBuild();
            if(XmlNode *DisplayVersion=Application->findNode("DisplayVersion"))DisplayVersion->data.setNum(1)[0]=cei().appBuild();
         }
      }
      if(!OverwriteOnChangeLoud(xml, build_path+"Assets/Nintendo Switch/Project.nmeta"))return false;
   }
   if(build_exe_type==EXE_NS)
   {
      Str data=build_path+"Assets/Nintendo Switch/Data/";

      // remove all unwanted
      CChar8 *allowed[]=
      {
         "Engine.pak",
         "Project.pak",
         "ShaderCache.pak",
      };
      DelExcept(data, allowed, Elms(allowed)); // remove all except 'allowed'

      FCreateDirs(data);

      // Engine.pak
      Str src=bin_path+"Mobile/Engine.pak", dest=data+"Engine.pak";
      if(cei().appEmbedEngineData()==1) // 2D only
      {
         if(!CreateEngineEmbedPak(src, dest, false))return false;
      }else
         if(!CopyFile(src, dest))return false;

      // ShaderCache.pak
      src=bin_path+"Nintendo/Switch ShaderCache.pak"; dest=data+"ShaderCache.pak";
      if(!FExistSystem(src))FDel(dest);else
      if(!VerifyPrecompiledShaderCache(src)){FDel(dest); Gui.msgBox(S, "Precompiled ShaderCache is outdated. Please regenerate it using \"Precompile Shaders\" tool, located inside \"Editor Source\\Tools\".");}else
      if(!CopyFile(src, dest))return false;
   }

   // universal manifest
   {
      XmlData xml;
      if(!xml.load("Code/Windows/Package.appxmanifest"))return ErrorRead("Code/Windows/Package.appxmanifest");

      if(XmlNode *package=xml.findNode("Package"))
      {
         if(XmlNode *identity=package->findNode("Identity"))
         {
            identity->getParam("Name"     ).setValue(Replace(cei().appPackage(), '_', '-')); // MS doesn't support '_' but supports '-'
            identity->getParam("Publisher").setValue(S+"CN="+CaseUp(cei().appMicrosoftPublisherID().asCanonical())); // MS might require case up
            identity->getParam("Version"  ).setValue(S+cei().appBuild()+".0.0.0");
         }
         if(XmlNode *properties=package->findNode("Properties"))
         {
            if(XmlNode *display_name=properties->findNode("DisplayName"))display_name->data.setNum(1)[0]=cei().appName();
            Str publisher_name=cei().appMicrosoftPublisherName(); if(publisher_name.is())if(XmlNode *publisher_display_name=properties->findNode("PublisherDisplayName"))publisher_display_name->data.setNum(1)[0]=publisher_name; // replace only if specified, because can't be empty
         }
         if(XmlNode *applications=package->findNode("Applications"))
            if(XmlNode *application=applications->findNode("Application"))
               if(XmlNode *visual_elements=application->findNode("uap:VisualElements"))
         {
            visual_elements->getParam("DisplayName").setValue(cei().appName());
            visual_elements->getParam("Description").setValue(cei().appName());

            XmlNode &rotations=visual_elements->getNode("uap:InitialRotationPreference");
            UInt          flag=cei().appSupportedOrientations();
                               rotations.nodes.del();
            if(flag&DIRF_UP   )rotations.nodes.New().setName("uap:Rotation").params.New().set("Preference", "portrait");
            if(flag&DIRF_DOWN )rotations.nodes.New().setName("uap:Rotation").params.New().set("Preference", "portraitFlipped");
            if(flag&DIRF_LEFT )rotations.nodes.New().setName("uap:Rotation").params.New().set("Preference", "landscape");
            if(flag&DIRF_RIGHT)rotations.nodes.New().setName("uap:Rotation").params.New().set("Preference", "landscapeFlipped");
         }
      }

      if(!OverwriteOnChangeLoud(xml, build_path+"Assets/Package.appxmanifest"))return false;
   }

   // solution
   if(!CopyFile("Code/Windows/Project.sln", build_path+"Project.sln"))return false;

 /*if(version==9)
   {
      XmlData xml;
      if(!xml.load("Code/Windows/project.vcproj"))return ErrorRead("Code/Windows/project.vcproj");
      if(XmlNode *proj=xml.findNode("VisualStudioProject"))
      {
         // project name
         proj->params.New().set("Name"         , build_project_name);
         proj->params.New().set("RootNamespace", build_project_name);

         // set files
         if(XmlNode *files=proj->findNode("Files"))StoreTree9(tree, *files);

         if(XmlNode *configs=proj->findNode("Configurations"))
            for(Int i=0; XmlNode *config=configs->findNode("Configuration", i); i++)
         {
            // set exe/dll
            if(build_exe_type==EXE_DLL)
               if(XmlParam *type=config->findParam("ConfigurationType"))
                  type->value="2"; // '1' is for EXE, '2' is for DLL

            for(Int i=0; XmlNode *tool=config->findNode("Tool", i); i++)
               if(XmlParam *name=tool->findParam("Name"))
            {
               if(name->value=="VCLinkerTool")
               {
                  if(build_exe_type==EXE_DLL)
                     if(XmlParam *out=tool->findParam("OutputFile"))
                        out->value="$(ProjectName).dll"; // or "$(ProjectName).exe"

                  // set libs
                  if(XmlParam *dependencies=tool->findParam("AdditionalDependencies"))
                  {
                     Int pos=TextPosI(dependencies->value, "EsenthelEngine");
                     if( pos>=0)
                     {
                        Int len=TextPosI(dependencies->value()+pos, ".lib"); len+=4;
                        Str lib; FREP(len)lib+=dependencies->value[pos+i];
                        dependencies->value.remove(pos, len+1);
                        dependencies->value.insert(0, S+'"'+bin_path_rel+lib+"\" ");
                     }
                     FREPA(libs)dependencies->value.space()+=S+'"'+libs[i]+'"';
                  }
               }else
               if(name->value=="VCCLCompilerTool")
               {
                  // set dirs
                  if(XmlParam *directories=tool->findParam("AdditionalIncludeDirectories"))
                  {
                     Str &dest=directories->value;
                     FREPA(dirs){if(dest.is() && dest.last()!=';')dest+=';'; dest.space()+=S+'"'+dirs[i]+'"';}
                  }
               }
            }
         }

         // save project
         build_project_file=build_path+"Project.vcproj";
         if(!OverwriteOnChangeLoud(xml, build_project_file))return false;
         return true;
      }
   }else*/
   if(version>=10 && version<=16) // #VisualStudio
   {
      XmlData xml, xml_f;
      if(!xml  .load(S+"Code/Windows/Project.vcxproj"        ))return ErrorRead(S+"Code/Windows/Project.vcxproj"        );
      if(!xml_f.load(S+"Code/Windows/Project.vcxproj.filters"))return ErrorRead(S+"Code/Windows/Project.vcxproj.filters");
      if(XmlNode *proj  =xml  .findNode("Project"))
      if(XmlNode *proj_f=xml_f.findNode("Project"))
      {
         // first adjust the "Engine.pak" path
         FREPA(proj->nodes)
         {
            XmlNode &node=proj->nodes[i]; if(node.name=="ItemGroup")FREPA(node.nodes)
            {
               XmlNode &sub=node.nodes[i]; if(sub.name=="Media")if(XmlParam *include=sub.findParam("Include"))if(include->value=="Engine.pak")
               {
                  include->value=bin_path_rel+"Universal\\Engine.pak"; goto found_engine_pak;
               }
            }
         }
      found_engine_pak:

         FREPA(proj_f->nodes)
         {
            XmlNode &node=proj_f->nodes[i]; if(node.name=="ItemGroup")FREPA(node.nodes)
            {
               XmlNode &sub=node.nodes[i]; if(sub.name=="Media")if(XmlParam *include=sub.findParam("Include"))if(include->value=="Engine.pak")
               {
                  include->value=bin_path_rel+"Universal\\Engine.pak"; goto found_engine_pak2;
               }
            }
         }
      found_engine_pak2:

         // project name
         for(Int i=0; XmlNode *prop=proj->findNode("PropertyGroup", i); i++)
            if(XmlParam *label=prop->findParam("Label"))if(label->value=="Globals")
         {
            prop->nodes.New().setName("RootNamespace").data.New()=build_project_name;
            prop->nodes.New().setName("ProjectName"  ).data.New()=build_project_name;

            // SDK #VisualStudio
            CChar8 *sdk=null; // https://en.wikipedia.org/wiki/Microsoft_Windows_SDK
            if(version==14)sdk="10.0.14393.795";else // latest SDK available for VS 2015
            if(version==15)sdk="10.0.17763.0"  ;     // latest SDK available for VS 2017
                                                     // VS 2019 does not require "WindowsTargetPlatformVersion" - when it's empty, then "latest SDK" is used, so don't specify it
            if(sdk)prop->getNode("WindowsTargetPlatformVersion").data.setNum(1)[0]=sdk;

            break;
         }

         // set files
         {
            XmlNode &group=proj->nodes.New().setName("ItemGroup");
            FREPA(build_files)
            {
               BuildFile &bf=build_files[i];
               if(bf.includeInProj())
               {
                  XmlNode &file=group.nodes.New().setName((bf.mode==BuildFile::SOURCE) ? "ClCompile" : (bf.mode==BuildFile::LIB) ? "Library" : (bf.mode==BuildFile::COPY) ? "CopyFileToFolders" : "None");
                  file.params.New().set("Include", bf.dest_proj_path);
               }
            }
         }

         // set filters
         {
            XmlNode &group=proj_f->nodes.New().setName("ItemGroup");
            StoreTree10(tree, group);
         }

         // set libs
         for(Int i=0; XmlNode *item=proj->findNode("ItemDefinitionGroup", i); i++)
            if(XmlNode *link        =item->findNode("Link"))
            if(XmlNode *dependencies=link->findNode("AdditionalDependencies"))
         {
            Str dest; FREPA(dependencies->data)dest.space()+=dependencies->data[i];
            Int pos=TextPosI(dest, "EsenthelEngine");
            if( pos>=0)
            {
               Int len=TextPosI(dest()+pos, ';');
               Str lib; FREP(len)lib+=dest[pos+i];
               dest.remove(pos, len+1);
               dest.insert(0, S+'"'+bin_path_rel+lib+"\";");
            }
            Memc<Str> *libs=&libs_win;
            if(XmlParam *condition=item->findParam("Condition"))
            {
               if(Contains(condition->value, "Emscripten", false, WHOLE_WORD_STRICT))libs=&libs_web;else
               if(Contains(condition->value, "NX64"      , false, WHOLE_WORD_STRICT))libs=&libs_ns ;
            }
            if(libs)FREPA(*libs){if(dest.is() && dest.last()!=';')dest+=';'; dest+=S+'"'+(*libs)[i]+'"';}
            Swap(dependencies->data.setNum(1)[0], dest);
         }

         // set dirs
         for(Int i=0; XmlNode *item=proj->findNode("ItemDefinitionGroup", i); i++)
            if(XmlNode *compile    =item   ->findNode("ClCompile"))
            if(XmlNode *directories=compile->findNode("AdditionalIncludeDirectories"))
         {
            Str dest; FREPA(directories->data)dest.space()+=directories->data[i];
            Memc<Str> *dirs=&dirs_win;
            if(XmlParam *condition=item->findParam("Condition"))
            {
               if(Contains(condition->value, "Emscripten", false, WHOLE_WORD_STRICT))dirs=&dirs_web;else
               if(Contains(condition->value, "NX64"      , false, WHOLE_WORD_STRICT))dirs=&dirs_ns;
            }
            if(dirs)FREPA(*dirs){if(dest.is() && dest.last()!=';')dest+=';'; dest+=S+'"'+(*dirs)[i]+'"';}
            Swap(directories->data.setNum(1)[0], dest);
         }

         // set exe/dll
         if(build_exe_type==EXE_DLL)
            for(Int i=0; XmlNode *prop=proj->findNode("PropertyGroup", i); i++)
               if(XmlNode *type=prop->findNode("ConfigurationType"))
                  type->data.setNum(1)[0]="DynamicLibrary"; // or "Application" or "StaticLibrary"

         // Platform toolset #VisualStudio
         CChar8 *platform_toolset=null, *platform_toolset_xp=null;
         if(version==10) platform_toolset=platform_toolset_xp="v100";else
         if(version==11){platform_toolset="v110"; platform_toolset_xp="v110_xp";}else
         if(version==12){platform_toolset="v120"; platform_toolset_xp="v120_xp";}else
         if(version==14){platform_toolset="v140"; platform_toolset_xp="v140_xp";}else
         if(version==15){platform_toolset="v141"; platform_toolset_xp="v141_xp";}else
         if(version==16){platform_toolset="v142"; platform_toolset_xp="v141_xp";}     // VS 2019 uses v141_xp from VS 2017

         if(platform_toolset)
            for(Int i=0; XmlNode *prop=proj->findNode("PropertyGroup", i); i++)
               if(C XmlParam *label=prop->findParam("Label"))if(label->value=="Configuration")
         {
          C XmlParam *condition=prop->findParam("Condition");
            Bool      keep     =(condition && Contains(condition->value, "Emscripten"));
            if(!keep) // keep default value
            {
               Bool winXP=(condition && Contains(condition->value, "Win32", false, WHOLE_WORD_STRICT) && ContainsAny(condition->value, u"GL DX9", false, WHOLE_WORD_STRICT)); // only Win32 DX9/GL is for WinXP
               XmlNode &PlatformToolset=prop->getNode("PlatformToolset");
               PlatformToolset.data.setNum(1)[0]=(winXP ? platform_toolset_xp : platform_toolset);
            }
         }

         // save project
         build_project_file=build_path+"Project.sln";
         if(!OverwriteOnChangeLoud(xml  , build_path+"Project.vcxproj"        ))return false;
         if(!OverwriteOnChangeLoud(xml_f, build_path+"Project.vcxproj.filters"))return false;
         return true;
      }
   }
   return false;
}
/******************************************************************************/
// Xcode projects operate on 96-bit ID's for elements in hex format, for example: "0B9F063216CD7D29006A0106" (for simplicity here only U64 is used, making upper bytes resulting as zeros)
static Str XcodeID(U64 id) {return TextHex(id, 24);} // 24 digits = 3 x UInt = 96-bit
struct XcodeFile
{
   Str name;
   U64 file, build, copy;

   XcodeFile() {file=build=copy=0;}
};
static void SetTreeID(Node<BuildFileElm> &node, U64 &file_id)
{
   node.id=(node.bf ? node.bf->xcode_file_id : file_id++);
   FREPA(node.children)SetTreeID(node.children[i], file_id);
}
static void SetGroups(Node<BuildFileElm> &node, Str &str)
{
// Sample:
//      080E96DDFE201D6D7F000001 /* Source */ = {
//         isa = PBXGroup;
//         children = (
//            0B9F065B16CD9232006A0106 /* Main.cpp */,
//         );
//         name = Source;
//         sourceTree = "<group>";
//      };
   if(!node.bf) // group
   {
      str+=S+"\t\t"+XcodeID(node.id)+" /* "+Replace(node.base_name, '*', '\0')+" */ = {\n";
      str+=S+"\t\t\tisa = PBXGroup;\n";
      str+=S+"\t\t\tchildren = (\n";
      FREPA(node.children) // keep order
      {
         str+=S+"\t\t\t\t"+XcodeID(node.children[i].id)+" /* "+Replace(node.children[i].base_name, '*', '\0')+" */,\n";
      }
      str+=S+"\t\t\t);\n";
      str+=S+"\t\t\tname = \""+CString(node.base_name)+"\";\n";
      str+=S+"\t\t\tsourceTree = \"<group>\";\n";
      str+=S+"\t\t};\n";
   }
   FREPA(node.children)SetGroups(node.children[i], str);
}
static Bool GetXcodeProjTextPos(Str &str, Int &pos, CChar8 *text)
{
   pos=TextPosI(str, text); if(pos<0)return Error(S+"Text \""+text+"\"\nwas not found in Xcode project template.");
   return true;
}
struct iOSIcon
{
   CChar8 *idiom, *size, *scale;
}iOSIcons[]=
{
   {"iphone", "20x20", "2x"},
   {"iphone", "20x20", "3x"},
   {"iphone", "29x29", "2x"},
   {"iphone", "29x29", "3x"},
   {"iphone", "40x40", "2x"},
   {"iphone", "40x40", "3x"},
   {"iphone", "60x60", "2x"},
   {"iphone", "60x60", "3x"},
   {"ipad", "20x20", "1x"},
   {"ipad", "20x20", "2x"},
   {"ipad", "29x29", "1x"},
   {"ipad", "29x29", "2x"},
   {"ipad", "40x40", "1x"},
   {"ipad", "40x40", "2x"},
   {"ipad", "76x76", "1x"},
   {"ipad", "76x76", "2x"},
   {"ipad", "83.5x83.5", "2x"},
   {"ios-marketing", "1024x1024", "1x"},
};
struct iOSSplash
{
   Bool    ipad;
   VecI2   size;
   CChar8 *scale, *min_os, *subtype=null;

   CChar8 *idiom()C {return ipad ? "ipad" : "iphone";}
   CChar8 *orientation()C {return size.x>size.y ? "landscape" : "portrait";}
}iOSSplashes[]=
{
   // ipad
   {true, VecI2(768, 1024), "1x", "7.0"},
   {true, VecI2(1024, 768), "1x", "7.0"},
   // iPhone
   {false, VecI2(640,  960), "2x", "7.0"           }, // iPhone Portrait iOS 7+ 2x
   {false, VecI2(640, 1136), "2x", "7.0", "retina4"}, // iPhone Portrait iOS 7+ Retina 4
   {false, VecI2(750, 1334), "2x", "8.0", "667h"   }, // iPhone Portrait iOS 8+ Retina HD 4.7"
};
Bool CodeEditor::generateXcodeProj()
{
   Str bin_path=BinPath();
   XmlData xml;
   Str str, add; Int pos;
   U64 file_id=0xEEC0C0A000000000;
   Node<BuildFileElm> tree;
   Memc<XcodeFile   > mac_assets, ios_images, mac_frameworks, ios_frameworks, mac_dylibs;
   FileText src; if(!src.read("Code/Apple/project.pbxproj"))return ErrorRead("Code/Apple/project.pbxproj"); src.getAll(str);

   build_project_file=build_path+"Project.xcodeproj";
   if(!FCreateDirs(build_project_file))return ErrorWrite(build_project_file);

   FCreateDirs(build_path+"Assets");

   if(!CopyFile("Code/Apple/iOS.xib", build_path+"Assets/iOS.xib"))return false;

   Memc<Str> libs_mac=GetFiles(cei().appLibsMac()),
             libs_ios=GetFiles(cei().appLibsiOS()),
             dirs    =GetFiles(cei().appDirsNonWindows());

   // embed engine data
   if(cei().appEmbedEngineData())
   {
      mac_assets.New().name="Assets/EngineEmbed.pak";
      if(!CreateEngineEmbedPak(S, build_path+"Assets/EngineEmbed.pak", true))return false;
   }

   // app data
   Bool exists; if(!CreateAppPak(build_path+"Assets/App.pak", exists))return false;
   if(  exists)mac_assets.New().name="Assets/App.pak";

   // Steam
   if(cei().appPublishSteamDll ())mac_dylibs.New().name=bin_path+ "libsteam_api.dylib";
   if(cei().appPublishOpenVRDll())mac_dylibs.New().name=bin_path+"libopenvr_api.dylib";

   // Images
   Image      icon; DateTime icon_time;
   if(GetIcon(icon,          icon_time))
   {
      // !! define images here, because we might point to them in the 'convert' !!
      Image portrait ; DateTime  portrait_time;
      Image landscape; DateTime landscape_time;
      Memc<ImageConvert> convert;
      CChar8 *rel="Assets/Icon.icns"; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).ICNS().clamp(256, 256);
      if(build_exe_type==EXE_IOS) // creating iOS icons/images is slow, so do this only when necessary
      {
         GetImages(portrait, portrait_time, landscape, landscape_time);

         FCreateDirs(build_path+"Assets/Images.xcassets/AppIcon.appiconset");
         FCreateDirs(build_path+"Assets/Images.xcassets/LaunchImage.launchimage");

         // icon
         {
            TextData Contents; TextNode &root=Contents.nodes.New();
            TextNode &images=root.nodes.New().setName("images");
            Memc<VecI2> sizes;
            FREPA(iOSIcons)
            {
             C auto &icon =iOSIcons[i];
               Vec2  sizef=TextVec2(Replace(icon.size, 'x', ','));
               Int   scale=TextInt(icon.scale);
               VecI2 size =Round(sizef*scale);
               sizes.binaryInclude(size);
               Str name=S+size.x+'x'+size.y+".png";
               TextNode &image=images.nodes.New();
               image.nodes.New().set("idiom"   , icon.idiom);
               image.nodes.New().set("size"    , icon.size);
               image.nodes.New().set("scale"   , icon.scale);
               image.nodes.New().set("filename", name);
            }
            TextNode &info=root.nodes.New().setName("info"); info.nodes.New().set("Version", 1); info.nodes.New().set("author", "Esenthel");
            FileText f; f.writeMem(); Contents.saveJSON(f); if(!OverwriteOnChangeLoud(f, build_path+"Assets/Images.xcassets/AppIcon.appiconset/Contents.json"))return false;
            FREPA(sizes)
            {
               auto size=sizes[i];
               Str  name=S+size.x+'x'+size.y+".png", full=build_path+"Assets/Images.xcassets/AppIcon.appiconset/"+name;
               if(CompareFile(FileInfoSystem(full).modify_time_utc, icon_time))convert.New().set(full, icon, icon_time).resize(size.x, size.y);
            }
         }

         // splash
         {
            TextData Contents; TextNode &root=Contents.nodes.New();
            TextNode &images=root.nodes.New().setName("images");
            Memc<VecI2> sizes;
            FREPA(iOSSplashes)
            {
             C auto &splash=iOSSplashes[i];
               VecI2 size  =splash.size;
               if(((size.x>size.y) ? landscape : portrait).is())
               {
                  sizes.binaryInclude(size);
                  Str name=S+size.x+'x'+size.y+".png";
                  TextNode &image=images.nodes.New();
                  image.nodes.New().set("idiom"      , splash.idiom());
                  image.nodes.New().set("orientation", splash.orientation());
                  image.nodes.New().set("filename"   , name);
                  image.nodes.New().set("extent"     , "full-screen");
                  if(CChar8 *v=splash.scale  )image.nodes.New().set("scale"                 , v);
                  if(CChar8 *v=splash.min_os )image.nodes.New().set("minimum-system-version", v);
                  if(CChar8 *v=splash.subtype)image.nodes.New().set("subtype"               , v);
               }
            }
            TextNode &info=root.nodes.New().setName("info"); info.nodes.New().set("Version", 1); info.nodes.New().set("author", "Esenthel");
            FileText f; f.writeMem(); Contents.saveJSON(f); if(!OverwriteOnChangeLoud(f, build_path+"Assets/Images.xcassets/LaunchImage.launchimage/Contents.json"))return false;
            FREPA(sizes)
            {
               auto size=sizes[i];
               Str  name=S+size.x+'x'+size.y+".png", full=build_path+"Assets/Images.xcassets/LaunchImage.launchimage/"+name;
             C DateTime &image_time=((size.x>size.y) ? landscape_time : portrait_time);
             C Image    &image     =((size.x>size.y) ? landscape      : portrait     );
               if(CompareFile(FileInfoSystem(full).modify_time_utc, image_time))convert.New().set(full, image, image_time).resizeFill(size.x, size.y);
            }
         }
      }

      convert.reverseOrder(); // start working from the biggest ones because they take the most time, yes this is correct
      MultiThreadedCall(convert, ImageConvert::Func);
      FREPA(convert)if(!convert[i].ok)return ErrorWrite(convert[i].dest);
   }

   Str app_package=Replace(cei().appPackage(), '_', '-'); // Apple does not support '_' but supports '-'

   // iOS.plist
   if(!xml.load("Code/Apple/iOS.plist"))return ErrorRead("Code/Apple/iOS.plist");
   if(XmlNode *plist=xml   .findNode("plist"))
   if(XmlNode *dict =plist->findNode("dict" ))FREPA(dict->nodes)if(dict->nodes[i].name=="key" && InRange(i+1, dict->nodes))
   {
      XmlNode &node =dict->nodes[i  ];
      XmlNode &value=dict->nodes[i+1];
      Str key; FREPA(node.data)key.space()+=node.data[i];
      if(key=="CFBundleDisplayName")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appName();
      }else
      if(key=="CFBundleVersion" || key=="CFBundleShortVersionString")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appBuild();
      }else
      if(key=="UISupportedInterfaceOrientations")
      {
         value.setName("array").nodes.del();
         UInt flag=cei().appSupportedOrientations();
         if(flag&DIRF_UP   )value.nodes.New().setName("string").data.add("UIInterfaceOrientationPortrait");
         if(flag&DIRF_DOWN )value.nodes.New().setName("string").data.add("UIInterfaceOrientationPortraitUpsideDown");
         if(flag&DIRF_LEFT )value.nodes.New().setName("string").data.add("UIInterfaceOrientationLandscapeRight");
         if(flag&DIRF_RIGHT)value.nodes.New().setName("string").data.add("UIInterfaceOrientationLandscapeLeft");
      }else
      if(key=="NSLocationAlwaysUsageDescription" || key=="NSLocationWhenInUseUsageDescription")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appLocationUsageReason();
      }else
      if(key=="NSCalendarsUsageDescription")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]="Unknown";
      }else
      if(key=="FacebookAppID")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appFacebookAppID();
      }else
      if(key=="FacebookDisplayName")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appName();
      }else
      if(key=="GADApplicationIdentifier")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appAdMobAppIDiOS();
      }else
      if(key=="ChartboostAppID")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appChartboostAppIDiOS();
      }else
      if(key=="ChartboostAppSignature")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appChartboostAppSignatureiOS();
      }else
      if(key=="CFBundleURLTypes")
      {
         if(value.name=="array")
         {
            XmlNode &dict=value.getNode("dict");
            dict.nodes.setNum(2, 0); // reset previous elements
            dict.nodes[0].setName("key"  ).data.add("CFBundleURLSchemes");
            dict.nodes[1].setName("array").nodes.New().setName("string").data.add(S+"fb"+cei().appFacebookAppID());
         }
      }
   }
   if(!OverwriteOnChangeLoud(xml, build_path+"Assets/iOS.plist"))return false;

   // Mac.plist
   if(!xml.load("Code/Apple/Mac.plist"))return ErrorRead("Code/Apple/Mac.plist");
   if(XmlNode *plist=xml   .findNode("plist"))
   if(XmlNode *dict =plist->findNode("dict" ))FREPA(dict->nodes)if(dict->nodes[i].name=="key" && dict->nodes[i].data.elms() && InRange(i+1, dict->nodes))
   {
    C Str     &key  =dict->nodes[i].data[0];
      XmlNode &value=dict->nodes[i+1];
      if(key=="CFBundleDisplayName")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appName();
      }else
      if(key=="CFBundleVersion")
      {
         value.setName("string").nodes.del();
         value.data.setNum(1)[0]=cei().appBuild();
      }
   }
   if(!OverwriteOnChangeLoud(xml, build_path+"Assets/Mac.plist"))return false;

   str=Replace(str, "path = \"EsenthelEngine Mac.a\""          , UnixPath(S+"path = \""+bin_path+"EsenthelEngine Mac.a\""          ));
   str=Replace(str, "path = \"EsenthelEngine iOS.a\""          , UnixPath(S+"path = \""+bin_path+"EsenthelEngine iOS.a\""          ));
   str=Replace(str, "path = \"EsenthelEngine iOS Simulator.a\"", UnixPath(S+"path = \""+bin_path+"EsenthelEngine iOS Simulator.a\""));
   str=Replace(str, "path = \"Engine.pak\""                    , UnixPath(S+"path = \""+bin_path+"Mobile/Engine.pak\""             ));
   str=Replace(str, "/* ESENTHEL FRAMEWORK DIRS */"            , S+'"'+CString(S+'"'+UnixPath(bin_path)+'"')+"\",");
   str=Replace(str, "/* ESENTHEL LIBRARY DIRS */"              , S+'"'+CString(S+'"'+UnixPath(bin_path)+'"')+"\",");
   str=Replace(str, "PRODUCT_BUNDLE_IDENTIFIER = \"\";"        , S+"PRODUCT_BUNDLE_IDENTIFIER = \""+CString(app_package       )+"\";");
   str=Replace(str, "PRODUCT_NAME = \"\";"                     , S+"PRODUCT_NAME = \""             +CString(build_project_name)+"\";");
   str=Replace(str, "path = Mac.app;"                          , S+"path = \""                     +CString(build_project_name)+".app\";");
   str=Replace(str, "path = iOS.app;"                          , S+"path = \""                     +CString(build_project_name)+".app\";");
   if(apple_team_id.is())
   {
      str=Replace(str, "DevelopmentTeam = \"\";", S+"DevelopmentTeam = \""+CString(apple_team_id)+"\";", true, WHOLE_WORD_STRICT);
      str=Replace(str, "DEVELOPMENT_TEAM = \"\";", S+"DEVELOPMENT_TEAM = \""+CString(apple_team_id)+"\";", true, WHOLE_WORD_STRICT);
      str=Replace(str, "ProvisioningStyle = Manual;", "ProvisioningStyle = Automatic;", true, WHOLE_WORD_STRICT);
      str=Replace(str, "CODE_SIGN_IDENTITY = \"-\";", "CODE_SIGN_IDENTITY = \"Apple Development\";", true, WHOLE_WORD_STRICT);
      str=Replace(str, "CODE_SIGN_STYLE = Manual;", "CODE_SIGN_STYLE = Automatic;", true, WHOLE_WORD_STRICT);
   }

   {
      Str lib_dirs;

      FREPA(libs_mac){Str path=GetPath(libs_mac[i]); if(path.is())lib_dirs.line()+=S+'"'+CString(S+'"'+path+'"')+"\",";}
      str=Replace(str, "/* ESENTHEL MAC LIBRARY DIRS */", lib_dirs);

      lib_dirs.clear();

      FREPA(libs_ios){Str path=GetPath(libs_ios[i]); if(path.is())lib_dirs.line()+=S+'"'+CString(S+'"'+path+'"')+"\",";}
      str=Replace(str, "/* ESENTHEL IOS LIBRARY DIRS */", lib_dirs);
   }


// PBXFileReference, Sample:
//      0B80AFFF1238566900C08944 /* Main.cpp */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.cpp.cpp; name = Main.cpp; path = Source/Main.cpp; sourceTree = "<group>"; };
   if(!GetXcodeProjTextPos(str, pos, "/* End PBXFileReference section */"))return false;
   add.clear();
   REPA(build_files)
   {
      BuildFile &bf=build_files[i]; if(bf.mode==BuildFile::SOURCE)
      {
         bf.xcode_file_id=file_id++;
         bf.xcode_mac_id =file_id++;
         bf.xcode_ios_id =file_id++;
         add+=S+"\t\t"+XcodeID(bf.xcode_file_id)+" /* "+Replace(bf.dest_proj_path, '*', '\0')+" */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.cpp.cpp; name = \""+CString(GetBase(bf.dest_proj_path))+"\"; path = \""+CString(UnixPath(bf.dest_proj_path))+"\"; sourceTree = \"<group>\"; };\n";
         BuildTree(tree, bf, bf.dest_proj_path);
      }
   }
   REPA(mac_dylibs)
   {
      XcodeFile &file=mac_dylibs[i];
      file.file =file_id++;
      file.build=file_id++;
      file.copy =file_id++;
      add+=S+"\t\t"+XcodeID(file.file)+" /* "+file.name+" */ = {isa = PBXFileReference; lastKnownFileType = \"compiled.mach-o.dylib\"; name = \""+CString(GetBase(file.name))+"\"; path = \""+CString(UnixPath(file.name))+"\"; sourceTree = \"<absolute>\"; };\n";
   }
   REPA(mac_assets)
   {
      XcodeFile &file=mac_assets[i];
      file.file =file_id++;
      file.build=file_id++;
      add+=S+"\t\t"+XcodeID(file.file)+" /* "+file.name+" */ = {isa = PBXFileReference; lastKnownFileType = file; name = \""+CString(GetBase(file.name))+"\"; path = \""+CString(UnixPath(file.name))+"\"; sourceTree = \"<group>\"; };\n";
   }
   REPA(ios_images)
   {
      XcodeFile &file=ios_images[i];
      file.file =file_id++;
      file.build=file_id++;
      add+=S+"\t\t"+XcodeID(file.file)+" /* "+file.name+" */ = {isa = PBXFileReference; lastKnownFileType = image.png; name = \""+CString(GetBase(file.name))+"\"; path = \""+CString(UnixPath(file.name))+"\"; sourceTree = \"<group>\"; };\n";
   }
   FREPA(libs_mac)
   {
      XcodeFile &file=mac_frameworks.New();
      file.name =libs_mac[i];
      file.file =file_id++;
      file.build=file_id++;
      add+=S+"\t\t"+XcodeID(file.file)+" /* "+file.name+" */ = {isa = PBXFileReference; lastKnownFileType = archive.ar; name = \""+CString(GetBase(file.name))+"\"; path = \""+CString(UnixPath(file.name))+"\"; sourceTree = \"<absolute>\"; };\n";
   }
   FREPA(libs_ios)
   {
      XcodeFile &file=ios_frameworks.New();
      file.name =libs_ios[i];
      file.file =file_id++;
      file.build=file_id++;
      add+=S+"\t\t"+XcodeID(file.file)+" /* "+file.name+" */ = {isa = PBXFileReference; lastKnownFileType = archive.ar; name = \""+CString(GetBase(file.name))+"\"; path = \""+CString(UnixPath(file.name))+"\"; sourceTree = \"<absolute>\"; };\n";
   }
   str.insert(pos, add);

// PBXBuildFile, Sample:
//      0B9F063216CD7D29006A0106 /* Main.cpp in Sources */ = {isa = PBXBuildFile; fileRef = 0B80AFFF1238566900C08944 /* Main.cpp */; };
   if(!GetXcodeProjTextPos(str, pos, "/* End PBXBuildFile section */"))return false;
   add.clear();
   REPA(build_files)
   {
      BuildFile &bf=build_files[i]; if(bf.mode==BuildFile::SOURCE)
      {
         add+=S+"\t\t"+XcodeID(bf.xcode_mac_id)+" /* Mac: "+Replace(bf.dest_proj_path, '*', '\0')+" */ = {isa = PBXBuildFile; fileRef = "+XcodeID(bf.xcode_file_id)+" /* "+Replace(bf.dest_proj_path, '*', '\0')+" */; };\n";
         add+=S+"\t\t"+XcodeID(bf.xcode_ios_id)+" /* iOS: "+Replace(bf.dest_proj_path, '*', '\0')+" */ = {isa = PBXBuildFile; fileRef = "+XcodeID(bf.xcode_file_id)+" /* "+Replace(bf.dest_proj_path, '*', '\0')+" */; };\n";
      }
   }
   REPA(mac_assets)
   {
      XcodeFile &file=mac_assets[i];
      add+=S+"\t\t"+XcodeID(file.build)+" /* Mac: "+file.name+" */ = {isa = PBXBuildFile; fileRef = "+XcodeID(file.file)+" /* "+file.name+" */; };\n";
   }
   REPA(ios_images)
   {
      XcodeFile &file=ios_images[i];
      add+=S+"\t\t"+XcodeID(file.build)+" /* iOS: "+file.name+" */ = {isa = PBXBuildFile; fileRef = "+XcodeID(file.file)+" /* "+file.name+" */; };\n";
   }
   REPA(mac_dylibs)
   {
      XcodeFile &file=mac_dylibs[i];
      add+=S+"\t\t"+XcodeID(file.build)+" /* Mac: "+file.name+" */ = {isa = PBXBuildFile; fileRef = "+XcodeID(file.file)+" /* "+file.name+" */; };\n";
      add+=S+"\t\t"+XcodeID(file.copy )+" /* Mac: "+file.name+" */ = {isa = PBXBuildFile; fileRef = "+XcodeID(file.file)+" /* "+file.name+" */; settings = {ATTRIBUTES = (CodeSignOnCopy, ); }; };\n";
   }
   REPA(mac_frameworks)
   {
      XcodeFile &file=mac_frameworks[i];
      add+=S+"\t\t"+XcodeID(file.build)+" /* Mac: "+file.name+" */ = {isa = PBXBuildFile; fileRef = "+XcodeID(file.file)+" /* "+file.name+" */; };\n";
   }
   REPA(ios_frameworks)
   {
      XcodeFile &file=ios_frameworks[i];
      add+=S+"\t\t"+XcodeID(file.build)+" /* iOS: "+file.name+" */ = {isa = PBXBuildFile; fileRef = "+XcodeID(file.file)+" /* "+file.name+" */; };\n";
   }
   str.insert(pos, add);

   // list frameworks
   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL MAC FRAMEWORKS */"))return false;
   add.clear();
   REPA(mac_frameworks)
   {
      XcodeFile &file=mac_frameworks[i];
//            0B9F062816CD7C2B006A0106 /* Engine.pak in Resources */,
      add+=S+"\t\t\t\t"+XcodeID(file.build)+" /* "+file.name+" */,\n";
   }
   REPA(mac_dylibs)
   {
      XcodeFile &file=mac_dylibs[i];
//            0B9F062816CD7C2B006A0106 /* Engine.pak in Resources */,
      add+=S+"\t\t\t\t"+XcodeID(file.build)+" /* "+file.name+" */,\n";
   }
   str.insert(pos, add);

   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL IOS FRAMEWORKS */"))return false;
   add.clear();
   REPA(ios_frameworks)
   {
      XcodeFile &file=ios_frameworks[i];
//            0B9F062816CD7C2B006A0106 /* Engine.pak in Resources */,
      add+=S+"\t\t\t\t"+XcodeID(file.build)+" /* "+file.name+" */,\n";
   }
   str.insert(pos, add);

   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL ALL FRAMEWORKS */"))return false;
   add.clear();
   REPA(mac_frameworks)
   {
      XcodeFile &file=mac_frameworks[i];
//            0B9F062816CD7C2B006A0106 /* Engine.pak in Resources */,
      add+=S+"\t\t\t\t"+XcodeID(file.file)+" /* "+file.name+" */,\n";
   }
   REPA(mac_dylibs)
   {
      XcodeFile &file=mac_dylibs[i];
//            0B9F062816CD7C2B006A0106 /* Engine.pak in Resources */,
      add+=S+"\t\t\t\t"+XcodeID(file.file)+" /* "+file.name+" */,\n";
   }
   REPA(ios_frameworks)
   {
      XcodeFile &file=ios_frameworks[i];
//            0B9F062816CD7C2B006A0106 /* Engine.pak in Resources */,
      add+=S+"\t\t\t\t"+XcodeID(file.file)+" /* "+file.name+" */,\n";
   }
   str.insert(pos, add);

   // list dylibs
   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL MAC DYLIBS */"))return false;
   add.clear();
   REPA(mac_dylibs)
   {
      XcodeFile &file=mac_dylibs[i];
//            0B9F062816CD7C2B006A0106 /* Engine.pak in Resources */,
      add+=S+"\t\t\t\t"+XcodeID(file.copy)+" /* "+file.name+" */,\n";
   }
   str.insert(pos, add);

   // create folders ("groups")
   SetTreeID(tree, file_id);
   if(!GetXcodeProjTextPos(str, pos, "/* End PBXGroup section */"))return false;
   add.clear();
   FREPA(tree.children)SetGroups(tree.children[i], add);
   str.insert(pos, add);

   // list root elements
   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL ROOT */"))return false;
   add.clear();
   FREPA(tree.children)
   {
// Sample:
//            0B80AFFF1238566900C08944 /* Main.cpp */,
      Node<BuildFileElm> &node=tree.children[i];
      add+=S+"\t\t\t\t"+XcodeID(node.id)+" /* "+Replace(node.base_name, '*', '\0')+" */,\n";
   }
   str.insert(pos, add);

   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL H CHILDREN */"))return false;

// ESENTHEL ASSETS CHILDREN, Sample:
//            0B9F060916CD7B80006A0106 /* Icon.png */,
   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL ASSETS CHILDREN */"))return false;
   add.clear();
   REPA(mac_assets)
   {
      XcodeFile &file=mac_assets[i];
      add+=S+"\t\t\t\t"+XcodeID(file.file)+" /* "+file.name+" */,\n";
   }
   REPA(ios_images)
   {
      XcodeFile &file=ios_images[i];
      add+=S+"\t\t\t\t"+XcodeID(file.file)+" /* "+file.name+" */,\n";
   }
   str.insert(pos, add);

   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL MAC EMBED */"))return false;
   add.clear();
   REPA(mac_assets)
   {
      XcodeFile &file=mac_assets[i];
// Sample:
//            0B9F062816CD7C2B006A0106 /* Engine.pak in Resources */,
      add+=S+"\t\t\t\t"+XcodeID(file.build)+" /* "+file.name+" */,\n";
   }
   str.insert(pos, add);

   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL IOS EMBED */"))return false;
   add.clear();
   REPA(ios_images)
   {
      XcodeFile &file=ios_images[i];
// Sample:
//            0B9F062816CD7C2B006A0106 /* Engine.pak in Resources */,
      add+=S+"\t\t\t\t"+XcodeID(file.build)+" /* "+file.name+" */,\n";
   }
   str.insert(pos, add);

   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL MAC SOURCE */"))return false;
   add.clear();
   REPA(build_files)
   {
      BuildFile &bf=build_files[i]; if(bf.mode==BuildFile::SOURCE)
      {
// Sample:
//            0B9F063216CD7D29006A0106 /* Main.cpp in Sources */,
         add+=S+"\t\t\t\t"+XcodeID(bf.xcode_mac_id)+" /* "+Replace(bf.dest_proj_path, '*', '\0')+" */,\n";
      }
   }
   str.insert(pos, add);

   if(!GetXcodeProjTextPos(str, pos, "/* ESENTHEL IOS SOURCE */"))return false;
   add.clear();
   REPA(build_files)
   {
      BuildFile &bf=build_files[i]; if(bf.mode==BuildFile::SOURCE)
      {
// Sample:
//            0B9F063216CD7D29006A0106 /* Main.cpp in Sources */,
         add+=S+"\t\t\t\t"+XcodeID(bf.xcode_ios_id)+" /* "+Replace(bf.dest_proj_path, '*', '\0')+" */,\n";
      }
   }
   str.insert(pos, add);

   // dirs
   Str include_dirs; FREPA(dirs)include_dirs.line()+=S+'"'+CString(S+'"'+dirs[i]+'"')+"\",";
   str=Replace(str, "/* ESENTHEL INCLUDE DIRS */", include_dirs);

   // save 'pbxproj' file
   FileText f; SetFile(f, str, UTF_8_NAKED); if(!OverwriteOnChangeLoud(f, build_project_file+"/project.pbxproj"))return false; // UTF_8_NAKED must be used as Xcode will fail with Byte Order Mark

   // Xcode will not delete any useless existing files in App/Contents/Resources (what's even worse is that it will not replace old resources with new ones), on iOS files are not stored inside "Contents/Resources" but in root of the app folder, so generally just delete everything inside it
   FDelInside(build_exe);

   return true;
}
/******************************************************************************/
static Str UnixEncode(C Str &s)
{
   Str o; o.reserve(s.length());
   FREPA(s)
   {
      Char c=s[i];
      if(c=='\\')o+='/';else
      if(c==' ' || c=='#' || c=='&' || c=='(' || c==')' || c=='\'' || c=='`' || c=='=' || c==';'){o+='\\'; o+=c;}else
      if(c=='$')o+="$$";else
         o+=c;
   }
   return o;
}
/******************************************************************************/
Bool CodeEditor::generateAndroidProj()
{
   FCreateDirs(build_path+"Android/jni");
   FCreateDir (build_path+"Android/libs");
   FCreateDir (build_path+"Android/src");
   FCreateDirs(build_path+"Android/src/com/android/vending/billing");
   FCreateDirs(build_path+"Android/res/layout");
   FCreateDirs(build_path+"Android/res/values");
 //FCreateDirs(build_path+"Android/res/xml"); // needed for "android.support.v4.content.FileProvider"

   Str bin_path=BinPath(false),
   android_path=bin_path+"Android/";

   // assets
   {
      // remove all unwanted
      CChar8 *allowed[]=
      {
         "Engine.pak",
         "Project.pak",
      };
      DelExcept(build_path+"Android/assets", allowed, Elms(allowed)); // remove all except 'allowed'

      // Engine.pak
      FCreateDirs(build_path+"Android/assets");
      Str src=bin_path+"Mobile/Engine.pak", dest=build_path+"Android/assets/Engine.pak";
      if(cei().appEmbedEngineData()==1) // 2D only
      {
         if(!CreateEngineEmbedPak(src, dest, false))return false;
      }else
         if(!CopyFile(src, dest))return false;
   }

   // layouts
   if(!CopyFile("Code/Android/LoaderLayout.xml", build_path+"Android/res/layout/loader.xml"))return false;

   // resources
   {
       XmlData  xml;
       XmlNode &res=xml.nodes.New().setName("resources");
      {XmlNode &n  =res.nodes.New().setName("string"); n.params.New().set("name", "facebook_app_id"         ); n.data.add(       cei().appFacebookAppID());}
      {XmlNode &n  =res.nodes.New().setName("string"); n.params.New().set("name", "fb_login_protocol_scheme"); n.data.add(S+"fb"+cei().appFacebookAppID());}
      if(!OverwriteOnChangeLoud(xml, build_path+"Android/res/values/strings.xml"))return false;
   }
 /*// file paths needed for "android.support.v4.content.FileProvider"
   {
      XmlData  xml;
      XmlNode &paths=xml.nodes.New().setName("paths"); paths.params.New().set("xmlns:android", "http://schemas.android.com/apk/res/android");
     {XmlNode &path =paths.nodes.New().setName("external-files-path"); path.params.New().set("name", "AppDataPublic"); path.params.New().set("path", "/"       );} // app data public - getExternalFilesDir
     {XmlNode &path =paths.nodes.New().setName("external-path"      ); path.params.New().set("name", "Public"       ); path.params.New().set("path", "/"       );} // public          - getExternalStorageDirectory
     {XmlNode &path =paths.nodes.New().setName("root-path"          ); path.params.New().set("name", "Root"         ); path.params.New().set("path", "/storage");} // root            - to support SD cards, this also includes entire public data, except private which are in other folders such as "/data" ("/data/app" is APK and "/data/user" is APP PRIVATE DATA)
     if(!OverwriteOnChangeLoud(xml, build_path+"Android/res/xml/file_paths.xml"))return false;
   }*/

   // Android.mk
   Str load_libraries;
   {
      Str lib_path=GetRelativePath(build_path+"Android/jni", android_path);
      if(FullPath(lib_path) || Contains(lib_path, ' ')) // does not support full paths or paths with spaces - I've tried replacing spaces with "\ ", "$(space)", putting quotes around the full path "C:/Path 2/", and putting path in separate variable CUSTOM:=path and using it $(CUSTOM)/file, NONE of this worked !! so just copy files if needed and use relative path
      {
         lib_path=projects_build_path; lib_path.tailSlash(true); // set target to the same folder for all applications, which means that when using 'CopyFile' we will copy the EE android libs only once for all apps
         Str p;
       //#AndroidArchitecture
       //p=android_path+"EsenthelEngine-armeabi.a"    ; CopyFile(p, lib_path+SkipStartPath(p, android_path)); // copy from "Bin" to 'projects_build_path'
       //p=android_path+"EsenthelEngine-armeabi-v7a.a"; CopyFile(p, lib_path+SkipStartPath(p, android_path)); // copy from "Bin" to 'projects_build_path'
         p=android_path+"EsenthelEngine-arm64-v8a.a"  ; CopyFile(p, lib_path+SkipStartPath(p, android_path)); // copy from "Bin" to 'projects_build_path'
       //p=android_path+"EsenthelEngine-x86.a"        ; CopyFile(p, lib_path+SkipStartPath(p, android_path)); // copy from "Bin" to 'projects_build_path'
         lib_path=GetRelativePath(build_path+"Android/jni", lib_path);
      }
      lib_path.tailSlash(false);
      lib_path=UnixPath(lib_path);
      Memc<Str> libs=GetFiles(cei().appLibsAndroid()),
                dirs=GetFiles(cei().PLATFORM(appDirsWindows, appDirsNonWindows)());

      Str ext_libs, ext_static_lib_names, ext_shared_lib_names, include_dirs;
      FREPA(libs)
      {
         // libs can be specified to include "$(TARGET_ARCH_ABI)", for example "/path/$(TARGET_ARCH_ABI)/libXXX.so" or "/path/libXXX-$(TARGET_ARCH_ABI).so"
         if(Contains(libs[i], ' '))return Error(S+"Library path may not contain spaces.\n\""+libs[i]+'"');
         Str name=GetBaseNoExt(libs[i]);
         name=Replace(name, "-$(TARGET_ARCH_ABI)", "");
         name=Replace(name,  "$(TARGET_ARCH_ABI)", "");
         ext_libs.line()+=S+"# "+name;
         ext_libs.line()+=  "include $(CLEAR_VARS)";
         ext_libs.line()+=S+"LOCAL_MODULE := "+name;
         ext_libs.line()+=S+"LOCAL_SRC_FILES := "+libs[i];
         if(GetExt(libs[i])=="so") // shared lib
         {
            if(!Starts(name, "lib"))return Error(S+"Shared library file names must start with \"lib\".\n\""+libs[i]+'"');
            ext_shared_lib_names.space()+=name;
            ext_libs.line()+="include $(PREBUILT_SHARED_LIBRARY)";
            load_libraries.line()+=S+"System.loadLibrary(\""+SkipStart(name, "lib")+"\");";
         }else // static lib
         {
            ext_static_lib_names.space()+=name;
            ext_libs.line()+="include $(PREBUILT_STATIC_LIBRARY)";
         }
      }
      FREPA(dirs)
      {
         include_dirs.space()+=S+"-I"+UnixEncode(dirs[i]);
      }

      FileText ft; if(!ft.read("Code/Android/Android.mk"))return ErrorRead("Code/Android/Android.mk");
      Str data=ft.getAll();
      data=Replace(data, "ESENTHEL_LIB_PATH"        , lib_path            , true, WHOLE_WORD_STRICT);
      data=Replace(data, "EXTERNAL_LIBS"            , ext_libs            , true, WHOLE_WORD_STRICT);
      data=Replace(data, "EXTERNAL_STATIC_LIB_NAMES", ext_static_lib_names, true, WHOLE_WORD_STRICT);
      data=Replace(data, "EXTERNAL_SHARED_LIB_NAMES", ext_shared_lib_names, true, WHOLE_WORD_STRICT);
      data=Replace(data, "INCLUDE_DIRS"             , include_dirs        , true, WHOLE_WORD_STRICT);
      SetFile(ft, data, UTF_8_NAKED); // does not support UTF
      if(!OverwriteOnChangeLoud(ft, build_path+"Android/jni/Android.mk"))return false;
   }

   // Application.mk
   {
      FileText ft; ft.writeMem(UTF_8_NAKED); FileText src; if(!src.read("Code/Android/Application.mk"))return ErrorRead("Code/Android/Application.mk"); // does not support UTF
      for(Str s; !src.end(); )
      {
         src.fullLine(s);
         s=Replace(s, "RELEASE_CONDITION", build_debug ? "ifeq (false, true)" : "ifeq (true, true)", true, WHOLE_WORD_STRICT);
         s=Replace(s, "ABI", "arm64-v8a", true, WHOLE_WORD_STRICT); // #AndroidArchitecture "armeabi-v7a arm64-v8a x86"
         ft.putLine(s);
      }
      if(!OverwriteOnChangeLoud(ft, build_path+"Android/jni/Application.mk"))return false;
   }

   // Main.cpp
   {
      FileText main; main.writeMem();
      FREPA(build_files)
      {
         BuildFile &bf=build_files[i];
         if(bf.includeInProj() && bf.mode==BuildFile::SOURCE)
         {
            main.putLine(S+"#include \"../../"+UnixPath(bf.dest_proj_path)+'"');
         }
      }
      if(!OverwriteOnChangeLoud(main, build_path+"Android/jni/Main.cpp"))return false;
   }

   // icons
   Image      icon, notification_icon; DateTime icon_time, notification_icon_time;
   if(GetIcon(icon, icon_time))
   {
      File f; f.writeMem();
      FCreateDir(build_path+"Android/res"); icon.transparentToNeighbor();
      Memc<ImageConvert> convert;
      CChar8 *rel;

      // list images starting from the smallest
      rel="Android/res/drawable-ldpi/icon.png"  ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize( 36,  36);
      rel="Android/res/drawable-mdpi/icon.png"  ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize( 48,  48);
      rel="Android/res/drawable-hdpi/icon.png"  ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize( 72,  72);
      rel="Android/res/drawable-xhdpi/icon.png" ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize( 96,  96);
      rel="Android/res/drawable-xxhdpi/icon.png"; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, icon_time))convert.New().set(build_path+rel, icon, icon_time).resize(144, 144);

      // https://developer.android.com/guide/practices/ui_guidelines/icon_design_status_bar.html
      GetNotificationIcon(notification_icon, notification_icon_time, icon, icon_time);
      rel="Android/res/drawable-ldpi/notification.png"  ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, notification_icon_time))convert.New().set(build_path+rel, notification_icon, notification_icon_time).resize(18, 18);
      rel="Android/res/drawable-mdpi/notification.png"  ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, notification_icon_time))convert.New().set(build_path+rel, notification_icon, notification_icon_time).resize(24, 24);
      rel="Android/res/drawable-hdpi/notification.png"  ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, notification_icon_time))convert.New().set(build_path+rel, notification_icon, notification_icon_time).resize(36, 36);
      rel="Android/res/drawable-xhdpi/notification.png" ; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, notification_icon_time))convert.New().set(build_path+rel, notification_icon, notification_icon_time).resize(48, 48);
      rel="Android/res/drawable-xxhdpi/notification.png"; if(CompareFile(FileInfoSystem(build_path+rel).modify_time_utc, notification_icon_time))convert.New().set(build_path+rel, notification_icon, notification_icon_time).resize(72, 72);

      convert.reverseOrder(); // start working from the biggest ones because they take the most time, yes this is correct
      MultiThreadedCall(convert, ImageConvert::Func);
      FREPA(convert)if(!convert[i].ok)return ErrorWrite(convert[i].dest);
   }

   Str  app_package=AndroidPackage(cei().appPackage());
   Bool chartboost          =(cei().appChartboostAppIDGooglePlay().is() && cei().appChartboostAppSignatureGooglePlay().is()),
        google_play_services=(cei().appAdMobAppIDGooglePlay     ().is() || chartboost),
        facebook            =(cei().appFacebookAppID()!=0);

   // AndroidManifest.xml
   XmlData xml;
   if(!xml.load("Code/Android/AndroidManifest.xml"))return ErrorRead("Code/Android/AndroidManifest.xml");
   if(XmlNode *manifest=xml.findNode("manifest"))
   {
      manifest->getParam("package"                ).value=app_package;
      manifest->getParam("android:versionCode"    ).value=cei().appBuild();
      manifest->getParam("android:versionName"    ).value=cei().appBuild();
      manifest->getParam("android:installLocation").value=((cei().appPreferredStorage()==STORAGE_EXTERNAL) ? "preferExternal" : (cei().appPreferredStorage()==STORAGE_AUTO) ? "auto" : "internalOnly");
      XmlNode &application=manifest->getNode("application");
      {
         if(icon.is())application.getParam("android:icon" ).value="@drawable/icon";
                      application.getParam("android:label").value=CString(cei().appName()); // android expects this as a C String

         // iterate activities
         REPA(application.nodes)
         {
            XmlNode &node=application.nodes[i]; if(node.name=="activity")if(XmlParam *name=node.findParam("android:name"))
            {
               if(name->value=="EsenthelActivity" || name->value=="LoaderActivity")
               {
                  node.getParam("android:label").value=CString(cei().appName()); // android expects this as a C String

                  // orientations
                  {
                     UInt flag     =cei().appSupportedOrientations();
                     Bool landscape=FlagTest(flag, DIRF_X),
                          portrait =FlagTest(flag, DIRF_Y);
                     CChar8    *orn=null;
                     if( flag==(DIRF_X|DIRF_UP)  )orn="sensor"         ;else // up/left/right no down
                     if((flag&DIRF_X)==DIRF_RIGHT)orn="landscape"      ;else // only one horizontal
                     if((flag&DIRF_Y)==DIRF_UP   )orn="portrait"       ;else // only one vertical
                     if( landscape && !portrait  )orn="sensorLandscape";else
                     if(!landscape &&  portrait  )orn="sensorPortrait" ;else
                                                  orn="fullSensor"     ;
                     node.getParam("android:screenOrientation").value=orn;
                  }
               }
            }
         }
         // needed for opening files through "content://" instead of deprecated "file://"
         {
            XmlNode &n=application.nodes.New().setName("provider" ); n.params.New().set("android:name", "EsenthelActivity$FileProvider"      ); n.params.New().set("android:authorities", app_package+".fileprovider"); n.params.New().set("android:exported", "false"); n.params.New().set("android:grantUriPermissions", "true");
          //XmlNode &m=          n.nodes.New().setName("meta-data"); m.params.New().set("android:name", "android.support.FILE_PROVIDER_PATHS"); m.params.New().set("android:resource", "@xml/file_paths"); // needed for "android.support.v4.content.FileProvider"
         }
         Str s;
         if(google_play_services){XmlNode &n=application.nodes.New().setName("meta-data"); n.params.New().set("android:name", "com.google.android.gms.version"); n.params.New().set("android:value", "@integer/google_play_services_version");}
         s=cei().appAdMobAppIDGooglePlay(); if(s.is())
         {
            {XmlNode &n=application.nodes.New().setName("activity" ); n.params.New().set("android:name", "com.google.android.gms.ads.AdActivity"    ); n.params.New().set("android:configChanges", "keyboard|keyboardHidden|orientation|screenLayout|uiMode|screenSize|smallestScreenSize");}
            {XmlNode &n=application.nodes.New().setName("meta-data"); n.params.New().set("android:name", "com.google.android.gms.ads.APPLICATION_ID"); n.params.New().set("android:value", s);}
         }
         if(chartboost)
         {
            XmlNode &n=application.nodes.New().setName("activity");
            n.params.New().set("android:name", "com.chartboost.sdk.CBImpressionActivity");
            n.params.New().set("android:excludeFromRecents", "true");
            n.params.New().set("android:hardwareAccelerated", "true");
            n.params.New().set("android:theme", "@android:style/Theme.Translucent.NoTitleBar.Fullscreen");
            n.params.New().set("android:configChanges", "keyboardHidden|orientation|screenSize");
         }
         if(ULong id=cei().appFacebookAppID())
         {
            {XmlNode &n=application.nodes.New().setName("meta-data"); n.params.New().set("android:name", "com.facebook.sdk.ApplicationId"      ); n.params.New().set("android:value", "@string/facebook_app_id"/*id*/);}
            {XmlNode &n=application.nodes.New().setName("activity" ); n.params.New().set("android:name", "com.facebook.FacebookActivity"       ); n.params.New().set("android:configChanges", "keyboard|keyboardHidden|screenLayout|screenSize|orientation"); n.params.New().set("android:label", CString(cei().appName()));} // android expects this as a C String
            {XmlNode &n=application.nodes.New().setName("provider" ); n.params.New().set("android:name", "com.facebook.FacebookContentProvider"); n.params.New().set("android:authorities", S+"com.facebook.app.FacebookContentProvider"+id); n.params.New().set("android:exported", "true");}
            {XmlNode &n=application.nodes.New().setName("activity" ); n.params.New().set("android:name", "com.facebook.CustomTabActivity"      ); n.params.New().set("android:exported", "true");
             XmlNode &intent_filter=n.nodes.New().setName("intent-filter");
             intent_filter.nodes.New().setName("action"  ).params.New().set("android:name"  , "android.intent.action.VIEW");
             intent_filter.nodes.New().setName("category").params.New().set("android:name"  , "android.intent.category.DEFAULT");
             intent_filter.nodes.New().setName("category").params.New().set("android:name"  , "android.intent.category.BROWSABLE");
             intent_filter.nodes.New().setName("data"    ).params.New().set("android:scheme", "@string/fb_login_protocol_scheme"/*S+"fb"+id*/);
            }
         }
      }
   }
   if(!OverwriteOnChangeLoud(xml, build_path+"Android/AndroidManifest.xml"))return false;

   android_path="Code/Android/"; // this is inside "Editor.pak"
   Str       android_libs_path=Str(projects_build_path).tailSlash(true)+"_Android_\\"; FCreateDir(android_libs_path); // path where to store Android libs "_Build_\_Android_\"
   Memc<Str> android_libs, jars;
   if(google_play_services)
   {
      android_libs.add("play-services-base"); jars.add("play-services-base"); // core, needed by "ads-lite"
      android_libs.add("play-services-basement"); jars.add("play-services-basement"); // core resources, needed by "ads-lite"
      android_libs.add("play-services-ads-lite"); jars.add("play-services-ads-lite"); // needed by AdMob and Chartboost
    //android_libs.add("play-services-ads"); jars.add("play-services-ads");
   }
 //"play-services-auth-base", // login/authentication
 //"play-services-drive", // google drive
#if 1
   android_libs.add("play-apk-expansion"); // allows downloading APK expansions
   android_libs.add("play-licensing"); // needed for "play-apk-expansion"
#else
   android_libs.add("market_apk_expansion"); // allows downloading APK expansions
   android_libs.add("market_licensing"); // needed for "market_apk_expansion"
#endif
   if(facebook)
   {
      android_libs.add("facebook-share"); jars.add("facebook-share");
      android_libs.add("facebook-login"); jars.add("facebook-login");
      android_libs.add("facebook-common"); jars.add("facebook-common");
      android_libs.add("facebook-core"); jars.add("facebook-core");

      android_libs.add("appcompat-v7"); jars.add("appcompat-v7"); // needed by Facebook (without it we get error: "facebook-common\res\values\values.xml:72: error: Error retrieving parent for item: No resource found that matches the given name '@style/Theme.AppCompat.NoActionBar'.")
      android_libs.add("cardview-v7"); jars.add("cardview-v7"); // needed by Facebook
    //android_libs.add("support-v4"); jars.add("support-v4");
   }

   // local.properties
   FileText local; local.writeMem(UTF_8_NAKED); local.putLine(S+"sdk.dir="+UnixPath(android_sdk));
   if(!OverwriteOnChangeLoud(local, build_path+"Android/local.properties"))return false;

   // project.properties
   {
      FileText project; project.writeMem(UTF_8_NAKED);
      project.putLine("target=android-28");
      // libraries (such as Google Play Services, Facebook, etc) always need to be copied
      // because while building, some files get modified/added which would change the "Editor/Bin" in conflict with auto update functionality, and additionally "local.properties" needs to be manually generated
      FREPA(android_libs) // process in order
      {
         Str src_path=android_path+android_libs[i], dest_path=android_libs_path+android_libs[i], dest_path_local_properties;
         if(C PaksFile *pf=Paks.find(src_path)){if(!FCopy(*pf->pak, *pf->file, dest_path, FILE_OVERWRITE_DIFFERENT))return ErrorWrite(dest_path);}else return ErrorRead(src_path); // copy from "Editor.pak" to 'android_libs_path'
         project.putLine(S+"android.library.reference."+(i+1)+"="+UnixPath(GetRelativePath(build_path+"Android", dest_path)));
         if(!OverwriteOnChangeLoud(local, dest_path+"/local.properties"))return false; // this file is not included in "Editor.pak", it's skipped in 'FilterEditorPak' function from "Esenthel Builder"
         FCreateDir(dest_path+"/src"); // an empty folder "src" is required in each library, or else compilation will fail, we can't just create them in "Esenthel\Editor Data\Code\Android" because Git doesn't allow empty folders
      }
      if(!OverwriteOnChangeLoud(project, build_path+"Android/project.properties"))return false;
   }
   // fixup "facebook-core/AndroidManifest.xml" TODO: Warning: this will trigger rewrite everytime compilation is started (first file is copied above inside 'FCopy' and then replaced below)
   if(facebook)
   {
      Str path=android_libs_path+"facebook-core/AndroidManifest.xml";
      FileText ft; if(!ft.read(path))return ErrorRead(path);
      Str data=ft.getAll();
      data=Replace(data, "${applicationId}", CString(app_package), true, WHOLE_WORD_STRICT);
      SetFile(ft, data, UTF_8_NAKED);
      if(!OverwriteOnChangeLoud(ft, path))return false;
   }

   if(chartboost)jars.add("chartboost");

   FREPA(jars)if(!CopyFile(android_path+jars[i]+"/classes.jar", build_path+"Android/libs/"+jars[i]+".jar"))return false;

   // build.xml
   if(!xml.load("Code/Android/build.xml"))return ErrorRead("Code/Android/build.xml");
   if(XmlNode *project=xml.findNode("project"))
   {
      project->getParam("name").value=build_project_name;
   }
   if(!OverwriteOnChangeLoud(xml, build_path+"Android/build.xml"))return false;

   // IInAppBillingService.aidl
   if(!CopyFile("Code/Android/IInAppBillingService.aidl", build_path+"Android/src/com/android/vending/billing/IInAppBillingService.aidl"))return false;

   // EsenthelActivity.java
   {
      FileText ft; if(!ft.read("Code/Android/EsenthelActivity.java"))return ErrorRead("Code/Android/EsenthelActivity.java");
      Str data=ft.getAll(), s;
      data=Replace(data, "EE_PACKAGE"              , app_package             , true, WHOLE_WORD_STRICT);
      data=Replace(data, "EE_APP_NAME"             , CString(cei().appName()), true, WHOLE_WORD_STRICT);
      s=cei().appGooglePlayLicenseKey();
      data=Replace(data, "EE_LICENSE_KEY"          , s                 , true, WHOLE_WORD_STRICT);
      data=Replace(data, "LICENSE_KEY_BEGIN"       , s.is() ? "" : "/*", true, WHOLE_WORD_STRICT);
      data=Replace(data, "LICENSE_KEY_END"         , s.is() ? "" : "*/", true, WHOLE_WORD_STRICT);
      s=cei().appAdMobAppIDGooglePlay();
      data=Replace(data, "ADMOB_APP_ID"            , CString(s)            , true, WHOLE_WORD_STRICT);
      data=Replace(data, "ADMOB_BEGIN"             , s.is()     ? "" : "/*", true, WHOLE_WORD_STRICT);
      data=Replace(data, "ADMOB_END"               , s.is()     ? "" : "*/", true, WHOLE_WORD_STRICT);
      data=Replace(data, "CHARTBOOST_BEGIN"        , chartboost ? "" : "/*", true, WHOLE_WORD_STRICT);
      data=Replace(data, "CHARTBOOST_END"          , chartboost ? "" : "*/", true, WHOLE_WORD_STRICT);
      data=Replace(data, "CHARTBOOST_APP_ID"       , CString(cei().appChartboostAppIDGooglePlay       ()), true, WHOLE_WORD_STRICT);
      data=Replace(data, "CHARTBOOST_APP_SIGNATURE", CString(cei().appChartboostAppSignatureGooglePlay()), true, WHOLE_WORD_STRICT);
      data=Replace(data, "FACEBOOK_BEGIN"          , facebook   ? "" : "/*", true, WHOLE_WORD_STRICT);
      data=Replace(data, "FACEBOOK_END"            , facebook   ? "" : "*/", true, WHOLE_WORD_STRICT);
      data=Replace(data, "EE_LOAD_LIBRARIES"       , load_libraries        , true, WHOLE_WORD_STRICT);
      SetFile(ft, data, UTF_8_NAKED);
      if(!OverwriteOnChangeLoud(ft, build_path+"Android/src/EsenthelActivity.java"))return false;
   }
   // LoaderActivity.java
   {
      FileText ft; if(!ft.read("Code/Android/LoaderActivity.java"))return ErrorRead("Code/Android/LoaderActivity.java");
      Str data=ft.getAll();
      data=Replace(data, "EE_PACKAGE"           , app_package                    , true, WHOLE_WORD_STRICT);
      data=Replace(data, "EE_LICENSE_KEY"       , cei().appGooglePlayLicenseKey(), true, WHOLE_WORD_STRICT);
      data=Replace(data, "EE_DOWNLOAD_EXPANSION", TextBool(cei().appAndroidExpansion() && cei().appGooglePlayLicenseKey().is()), true, WHOLE_WORD_STRICT); // require license key provided, because otherwise the Java codes will crash throwing an exception
      SetFile(ft, data, UTF_8_NAKED);
      if(!OverwriteOnChangeLoud(ft, build_path+"Android/src/LoaderActivity.java"))return false;
   }
   if(!CopyFile("Code/Android/Native.java", build_path+"Android/src/Native.java"))return false;
   if(!CopyFile("Code/Android/Base64.java", build_path+"Android/src/Base64.java"))return false;

   return true;
}
/******************************************************************************/
Bool CodeEditor::generateLinuxMakeProj()
{
   FCreateDirs(build_path+"nbproject");
   FileText f; Str s;
   Str bin_path=BinPath(),
       EE_APP_NAME=UnixEncode(GetBase(build_exe)),
       EE_LIB_PATH=UnixEncode(bin_path+"EsenthelEngine.a"),
       EXTERNAL_LIBS,
       EE_HEADER_PATH,
       EE_OBJ_FILES,
       EE_CPP_FILES_Debug, EE_CPP_FILES_Release;

   Memc<Str> libs=GetFiles(cei().appLibsLinux());
   if(cei().appPublishSteamDll ())libs.add("Bin/libsteam_api.so" ); // this must be relative to the EXE because this path will be embedded in the executable ("Bin/" is needed because without it building will fail)
   if(cei().appPublishOpenVRDll())libs.add("Bin/libopenvr_api.so"); // this must be relative to the EXE because this path will be embedded in the executable ("Bin/" is needed because without it building will fail)
   FREPA(libs)EXTERNAL_LIBS.space()+=UnixEncode(libs[i]);

   Memc<Str> dirs=GetFiles(cei().appDirsNonWindows());
   FREPA(dirs)EE_HEADER_PATH.space()+=S+"-I"+UnixEncode(dirs[i]);

   FREPA(build_files)
   {
      BuildFile &bf=build_files[i];
      if(bf.includeInProj() && bf.mode==BuildFile::SOURCE)
      {
         Str obj_name=S+"${OBJECTDIR}/"+UnixEncode(GetBase(bf.ext_not      ))+".o",
             src_name=                  UnixEncode(        bf.dest_proj_path);

         EE_OBJ_FILES+='\t'; EE_OBJ_FILES+=obj_name;
         if(InRange(i+1, build_files))EE_OBJ_FILES+=" \\";
         EE_OBJ_FILES+='\n';

         EE_CPP_FILES_Debug+=obj_name+": "+src_name+'\n'; // ${OBJECTDIR}/_ext/1728301206/Auto.o: ../Source/Auto.cpp
         EE_CPP_FILES_Debug+="\t${MKDIR} -p ${OBJECTDIR}\n"; // ${MKDIR} -p ${OBJECTDIR}
         EE_CPP_FILES_Debug+="\t${RM} \"$@.d\"\n"; // ${RM} "$@.d"
         EE_CPP_FILES_Debug+=S+"\t$(COMPILE.cc) -g -DDEBUG=1 -I. "+EE_HEADER_PATH+" -std=c++17 -MMD -MP -MF \"$@.d\" -o "+obj_name+' '+src_name+'\n'; // $(COMPILE.cc) -g -DDEBUG -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/1728301206/Auto.o ../Source/Auto.cpp
         EE_CPP_FILES_Debug+='\n';

         EE_CPP_FILES_Release+=obj_name+": "+src_name+'\n'; // ${OBJECTDIR}/_ext/1728301206/Auto.o: ../Source/Auto.cpp
         EE_CPP_FILES_Release+="\t${MKDIR} -p ${OBJECTDIR}\n"; // ${MKDIR} -p ${OBJECTDIR}
         EE_CPP_FILES_Release+="\t${RM} \"$@.d\"\n"; // ${RM} "$@.d"
         EE_CPP_FILES_Release+=S+"\t$(COMPILE.cc) -DDEBUG=0 -O3 -I. "+EE_HEADER_PATH+" -std=c++17 -MMD -MP -MF \"$@.d\" -o "+obj_name+' '+src_name+'\n'; // $(COMPILE.cc) -O3 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/1728301206/Auto.o ../Source/Auto.cpp
         EE_CPP_FILES_Release+='\n';
      }
   }

   if(!f.read("Code/Linux/Makefile"))return ErrorRead("Code/Linux/Makefile"); f.getAll(s);
   s=Replace(s, "EE_HEADER_PATH", EE_HEADER_PATH, true, WHOLE_WORD_STRICT); SetFile(f, s, UTF_8_NAKED); if(!OverwriteOnChangeLoud(f, build_path+"Makefile"))return false;

   if(!f.read("Code/Linux/nbproject/Makefile-impl.mk"))return ErrorRead("Code/Linux/nbproject/Makefile-impl.mk"); f.getAll(s);
   s=Replace(s, "EE_APP_NAME", EE_APP_NAME, true, WHOLE_WORD_STRICT); SetFile(f, s, UTF_8_NAKED); if(!OverwriteOnChangeLoud(f, build_path+"nbproject/Makefile-impl.mk"))return false;

   if(!f.read("Code/Linux/nbproject/Makefile-variables.mk"))return ErrorRead("Code/Linux/nbproject/Makefile-variables.mk"); f.getAll(s);
   s=Replace(s, "EE_APP_NAME", EE_APP_NAME, true, WHOLE_WORD_STRICT); SetFile(f, s, UTF_8_NAKED); if(!OverwriteOnChangeLoud(f, build_path+"nbproject/Makefile-variables.mk"))return false;

   if(!f.read("Code/Linux/nbproject/Package-Debug.bash"))return ErrorRead("Code/Linux/nbproject/Package-Debug.bash"); f.getAll(s);
   s=Replace(s, "EE_APP_NAME", EE_APP_NAME, true, WHOLE_WORD_STRICT); SetFile(f, s, UTF_8_NAKED); if(!OverwriteOnChangeLoud(f, build_path+"nbproject/Package-Debug.bash"))return false;

   if(!f.read("Code/Linux/nbproject/Package-Release.bash"))return ErrorRead("Code/Linux/nbproject/Package-Release.bash"); f.getAll(s);
   s=Replace(s, "EE_APP_NAME", EE_APP_NAME, true, WHOLE_WORD_STRICT); SetFile(f, s, UTF_8_NAKED); if(!OverwriteOnChangeLoud(f, build_path+"nbproject/Package-Release.bash"))return false;

   if(!f.read("Code/Linux/nbproject/Makefile-Debug.mk"))return ErrorRead("Code/Linux/nbproject/Makefile-Debug.mk"); f.getAll(s);
   s=Replace(s, "EE_APP_NAME"   , EE_APP_NAME       , true, WHOLE_WORD_STRICT);
   s=Replace(s, "EE_LIB_PATH"   , EE_LIB_PATH       , true, WHOLE_WORD_STRICT);
   s=Replace(s, "EE_HEADER_PATH", EE_HEADER_PATH    , true, WHOLE_WORD_STRICT);
   s=Replace(s, "EE_OBJ_FILES"  , EE_OBJ_FILES      , true, WHOLE_WORD_STRICT);
   s=Replace(s, "EE_CPP_FILES"  , EE_CPP_FILES_Debug, true, WHOLE_WORD_STRICT);
   s=Replace(s, "EXTERNAL_LIBS" , EXTERNAL_LIBS     , true, WHOLE_WORD_STRICT);
   SetFile(f, s, UTF_8_NAKED); if(!OverwriteOnChangeLoud(f, build_path+"nbproject/Makefile-Debug.mk"))return false;

   if(!f.read("Code/Linux/nbproject/Makefile-Release.mk"))return ErrorRead("Code/Linux/nbproject/Makefile-Release.mk"); f.getAll(s);
   s=Replace(s, "EE_APP_NAME"   , EE_APP_NAME         , true, WHOLE_WORD_STRICT);
   s=Replace(s, "EE_LIB_PATH"   , EE_LIB_PATH         , true, WHOLE_WORD_STRICT);
   s=Replace(s, "EE_HEADER_PATH", EE_HEADER_PATH      , true, WHOLE_WORD_STRICT);
   s=Replace(s, "EE_OBJ_FILES"  , EE_OBJ_FILES        , true, WHOLE_WORD_STRICT);
   s=Replace(s, "EE_CPP_FILES"  , EE_CPP_FILES_Release, true, WHOLE_WORD_STRICT);
   s=Replace(s, "EXTERNAL_LIBS" , EXTERNAL_LIBS       , true, WHOLE_WORD_STRICT);
   SetFile(f, s, UTF_8_NAKED); if(!OverwriteOnChangeLoud(f, build_path+"nbproject/Makefile-Release.mk"))return false;

#if LINUX // do this only on Linux because only on Linux SDK the *.so files are available
   Str bin_dest=build_path+"Bin/";
   #if PHYSX_DLL // PhysX DLL's
      if(cei().appPublishPhysxDll()) // this must be copied always (unlike for Windows where it's copied only for publishing), because on Windows we can specify a custom path for the DLL's, however on Linux it needs to be hardcoded to "./Bin/", so everytime we want to start an app, it needs to have the .so files in the Bin relative to the executable
      {
         FCreateDir(bin_dest);
         CChar8 *dlls[]=
         {
            "libPhysXGpu_64.so",
         };
         FREPA(dlls)if(!CopyFile(bin_path+dlls[i], bin_dest+dlls[i]))return false;
      }
   #endif
   // Steam DLL's
   if(cei().appPublishSteamDll()) // this must be copied always, because libs on Linux need to be hardcoded to "./Bin/", so everytime we want to start an app, it needs to have the .so files in the Bin relative to the executable
   {
      FCreateDir(bin_dest);
      CChar8 *dlls[]=
      {
         "libsteam_api.so",
      };
      FREPA(dlls)if(!CopyFile(bin_path+dlls[i], bin_dest+dlls[i]))return false;
   }
   // OpenVR DLL's
   if(cei().appPublishOpenVRDll()) // this must be copied always because libs on Linux need to be hardcoded to "./Bin/", so everytime we want to start an app, it needs to have the .so files in the Bin relative to the executable
   {
      FCreateDir(bin_dest);
      CChar8 *dlls[]=
      {
         "libopenvr_api.so",
      };
      FREPA(dlls)if(!CopyFile(bin_path+dlls[i], bin_dest+dlls[i]))return false;
   }
#endif

   // embed resources
   Image      icon; DateTime icon_time;
   if(GetIcon(icon,          icon_time))
   {
      File f; f.writeMem();
      BuildEmbed &be=build_embed.New().set(CC4('I', 'C', 'O', 'N'), build_path+"Assets/Icon.webp");
      if(CompareFile(FileInfoSystem(be.path).modify_time_utc, icon_time)){icon.ExportWEBP(f.reset()); if(!OverwriteOnChangeLoud(f, be.path))return false; FTimeUTC(be.path, icon_time);}
   }
   if(cei().appEmbedEngineData())
   {
      BuildEmbed &be=build_embed.New().set(CC4('P', 'A', 'K', 0), build_path+"Assets/EngineEmbed.pak");
      if(!CreateEngineEmbedPak(S, be.path, true))return false;
   }
   Str  app_pak_path=build_path+"Assets/App.pak";
   Bool exists; if(!CreateAppPak(app_pak_path, exists))return false;
   if(  exists)build_embed.New().set(CC4('P', 'A', 'K', 0), app_pak_path);

   // we need to recreate the exe everytime in case we're embedding app resources manually
   FDelFile(build_exe);

   return true;
}
static void StoreTree(Node<BuildFileElm> &nodes, Str &EE_APP_ITEMS)
{
   FREPA(nodes.children)
   {
      Node<BuildFileElm> &node=nodes.children[i];
      if(node.file())
      {
         EE_APP_ITEMS+=S+"<itemPath>"+XmlString(UnixPath(node.full_name))+"</itemPath>\n";
      }else
      {
         EE_APP_ITEMS+=S+"<logicalFolder name=\""+XmlString(node.base_name)+"\" displayName=\""+XmlString(node.base_name)+"\" projectFiles=\"true\">\n";
         StoreTree(node, EE_APP_ITEMS);
         EE_APP_ITEMS+="</logicalFolder>\n";
      }
   }
}
Bool CodeEditor::generateLinuxNBProj()
{
   if(generateLinuxMakeProj())
   {
      FileText f; Str s;
      Str bin_path=BinPath(),
          EE_APP_NAME     =XmlString(build_project_name),
          EE_APP_NAME_SAFE=XmlString(CleanNameForNetBeans(GetBase(build_exe))),
          EE_LIB_PATH=XmlString(UnixPath(bin_path+"EsenthelEngine.a")),
          EXTERNAL_LIBS,
          EE_HEADER_PATH,
          EE_APP_ITEMS,
          EE_APP_FILES;

      Node<BuildFileElm> tree; FREPA(build_files)
      {
         BuildFile &bf=build_files[i];
         if(bf.mode==BuildFile::SOURCE)
         {
            BuildTree(tree, bf, bf.dest_proj_path);
            EE_APP_FILES+=S+"<item path=\""+XmlString(UnixPath(bf.dest_proj_path))+"\" ex=\"false\" tool=\"1\" flavor2=\"0\" />\n"; // tool(CPP=1, H=3)
         }
      }
      StoreTree(tree, EE_APP_ITEMS);

      Memc<Str> libs=GetFiles(cei().appLibsLinux());
      if(cei().appPublishSteamDll ())libs.add(bin_path+ "libsteam_api.so");
      if(cei().appPublishOpenVRDll())libs.add(bin_path+"libopenvr_api.so");
      FREPA(libs)EXTERNAL_LIBS+=S+"<linkerLibFileItem>"+XmlString(libs[i])+"</linkerLibFileItem>\n";

      Memc<Str> dirs=GetFiles(cei().appDirsNonWindows());
      FREPA(dirs)EE_HEADER_PATH+=S+"<pElem>"+XmlString(dirs[i])+"</pElem>\n";

      if(!f.read("Code/Linux/nbproject/configurations.xml"))return ErrorRead("Code/Linux/nbproject/configurations.xml"); f.getAll(s);
      s=Replace(s, "EE_APP_NAME"   , EE_APP_NAME_SAFE, true, WHOLE_WORD_STRICT);
      s=Replace(s, "EE_LIB_PATH"   , EE_LIB_PATH     , true, WHOLE_WORD_STRICT);
      s=Replace(s, "EE_HEADER_PATH", EE_HEADER_PATH  , true, WHOLE_WORD_STRICT);
      s=Replace(s, "EE_APP_ITEMS"  , EE_APP_ITEMS    , true, WHOLE_WORD_STRICT);
      s=Replace(s, "EE_APP_FILES"  , EE_APP_FILES    , true, WHOLE_WORD_STRICT);
      s=Replace(s, "EXTERNAL_LIBS" , EXTERNAL_LIBS   , true, WHOLE_WORD_STRICT);
      SetFile(f, s); if(!OverwriteOnChangeLoud(f, build_path+"nbproject/configurations.xml"))return false;

      if(!f.read("Code/Linux/nbproject/project.xml"))return ErrorRead("Code/Linux/nbproject/project.xml"); f.getAll(s);
      s=Replace(s, "EE_APP_NAME", EE_APP_NAME, true, WHOLE_WORD_STRICT);
      SetFile(f, s); if(!OverwriteOnChangeLoud(f, build_path+"nbproject/project.xml"))return false;

      return true;
   }
   return false;
}
/******************************************************************************/
static EXPORT_MODE ExeToMode(EXE_TYPE exe)
{
   switch(exe)
   {
      default: return EXPORT_VS;

      case EXE_APK: return EXPORT_ANDROID;

      case EXE_MAC:
      case EXE_IOS: return EXPORT_XCODE;

      case EXE_LINUX: return EXPORT_LINUX_NETBEANS;
   }
}
Bool CodeEditor::Export(EXPORT_MODE mode, BUILD_MODE build_mode)
{
   stopBuild();

   build_exe_type=config_exe;
   switch(mode) // override EXE type when exporting
   {
      case EXPORT_LINUX_NETBEANS:
      case EXPORT_LINUX_MAKE    : build_exe_type=EXE_LINUX; break;
      case EXPORT_ANDROID       : build_exe_type=EXE_APK  ; break;
      case EXPORT_XCODE         : if(build_exe_type!=EXE_MAC && build_exe_type!=EXE_IOS)build_exe_type=EXE_MAC; break;
      // no need to check for VS as it will just fail with a message box about exe/dll/web support only
   }

   if(build_exe_type==EXE_DLL && (build_mode==BUILD_PLAY || build_mode==BUILD_DEBUG))build_mode=BUILD_BUILD;
 T.build_mode             =build_mode;
   build_debug            =((build_mode==BUILD_PUBLISH) ? false : config_debug); // set before 'verifyBuildPath' and before creating android project
   build_windows_code_sign=((build_mode==BUILD_PUBLISH) && build_exe_type==EXE_EXE && options.authenticode()/*cei().appWindowsCodeSign()*/);
   build_project_id       =cei().projectID();

   if(!verifyBuildPath())Error("No application selected or it has invalid name.");else
   {
      if(mode==EXPORT_EXE)mode=ExeToMode(build_exe_type);
      if(mode==EXPORT_VS)
      {
         // #VisualStudio
         if(!verifyVS())return false; // this must be checked to validate currently installed VS
       /*if(devenv_version.x== 9)mode=EXPORT_VS2008;else
         if(devenv_version.x==10)mode=EXPORT_VS2010;else
         if(devenv_version.x==11)mode=EXPORT_VS2012;else
         if(devenv_version.x==12)mode=EXPORT_VS2013;else*/
         if(devenv_version.x==14)mode=EXPORT_VS2015;else
         if(devenv_version.x==15)mode=EXPORT_VS2017;else
         if(devenv_version.x==16)mode=EXPORT_VS2019;else
            return false;
      }else
      if(mode==EXPORT_ANDROID       ){if(!verifyAndroid  ())return false;}else
      if(mode==EXPORT_XCODE         ){if(!verifyXcode    ())return false;}else
      if(mode==EXPORT_LINUX_MAKE    ){if(!verifyLinuxMake())return false;}else
      if(mode==EXPORT_LINUX_NETBEANS){if(!verifyLinuxMake())return false;} // ignore checks for NetBeans as we want to create the project even if there's no NetBeans installed

      Memc<Message> msgs;
      if(mode==EXPORT_TXT){if(generateTXT(msgs))return true;}else
      {
         init();

         Memc<Symbol*> sorted_classes;
         if(verifyBuildFiles(msgs))if(CodeEnvironment::VerifySymbols(msgs, sorted_classes))if(adjustBuildFiles())if(generateCPPH(sorted_classes, mode))
         {
            // #VisualStudio
            if(mode==EXPORT_CPP           )return true;
          /*if(mode==EXPORT_VS2008        )return generateVSProj     ( 9);
            if(mode==EXPORT_VS2010        )return generateVSProj     (10);
            if(mode==EXPORT_VS2012        )return generateVSProj     (11);
            if(mode==EXPORT_VS2013        )return generateVSProj     (12);*/
            if(mode==EXPORT_VS2015        )return generateVSProj     (14);
            if(mode==EXPORT_VS2017        )return generateVSProj     (15);
            if(mode==EXPORT_VS2019        )return generateVSProj     (16);
            if(mode==EXPORT_XCODE         )return generateXcodeProj    ();
            if(mode==EXPORT_ANDROID       )return generateAndroidProj  ();
            if(mode==EXPORT_LINUX_MAKE    )return generateLinuxMakeProj();
            if(mode==EXPORT_LINUX_NETBEANS)return generateLinuxNBProj  ();
         }
      }
      if(msgs.elms())
      {
         buildClear (    );
         buildNew   (msgs);
         buildUpdate(    );
      }
   }
   return false;
}
/******************************************************************************/
void CodeEditor::stopBuild()
{
   build_phase  =build_step =-1;
   build_phases =build_steps= 0;
   build_log       .clear();
   build_exe       .clear();
   build_embed     .clear();
   build_package   .clear();
   build_output    .clear();
   build_line_maps .clear();
   build_process   .del  ();
   build_project_id.zero ();
}
void CodeEditor::killBuild()
{
      build_process.stop();
   if(build_process.active())
   {
      if(!build_process.wait(100))build_process.kill();
      buildNew().set("Stopped on user request");
      buildUpdate();
   }
   stopBuild();
}
/******************************************************************************/
void CodeEditor::build(BUILD_MODE mode)
{
   if((mode==BUILD_PLAY || mode==BUILD_PUBLISH) && (config_exe==EXE_UWP || config_exe==EXE_IOS || config_exe==EXE_NS)){openIDE(); return;} // Play/Publish for WindowsNew and iOS must be done from the IDE

   if(Export(EXPORT_EXE, mode))
   {
      build_list.highlight_line=-1;
      build_list.highlight_time= 0;
      build_refresh=0;
      build_phase  =build_step =0;
      build_phases =build_steps=0;

      Int build_threads=Cpu.threads();
      if(build_exe_type==EXE_EXE || build_exe_type==EXE_DLL || build_exe_type==EXE_LIB || build_exe_type==EXE_UWP || build_exe_type==EXE_WEB || build_exe_type==EXE_NS)
      {
         build_phases=1+build_windows_code_sign;
         build_steps =3+build_windows_code_sign; FREPA(build_files)if(build_files[i].mode==BuildFile::SOURCE)build_steps++; // stdafx.cpp, linking, wait for end, *.cpp

         Str config=(build_debug ? "Debug" : "Release");
         if(build_exe_type==EXE_UWP)config+=" Universal";

         if(build_exe_type==EXE_NS )config+=" DX11";else // always use the same config for NS  because it uses    GL, warning: this must match codes above: (build_debug ? "Debug DX11/" : "Release DX11/")
         if(build_exe_type==EXE_WEB)config+=" DX11";else // always use the same config for WEB because it uses WebGL, warning: this must match codes above: (build_debug ? "Debug DX11/" : "Release DX11/")
         if(build_exe_type==EXE_UWP)config+=" DX11";else
                                    config+=" DX11"; // config_api

         Str platform=((build_exe_type==EXE_NS) ? "4) Nintendo Switch" : (build_exe_type==EXE_WEB) ? "3) Web" : /*config_32_bit ? "2) 32 bit" :*/ "1) 64 bit");

         if(build_exe_type==EXE_WEB) // currently WEB compilation is available only through VC++ 2010
         {
            Memc<VisualStudioInstallation> vs_installs; GetVisualStudioInstallations(vs_installs);
            REPA(vs_installs)if(vs_installs[i].ver.x==10)
            {
               Str devenv=Str(vs_installs[i].path).tailSlash(true)+"Common7\\IDE\\devenv.exe"   ; if(FExistSystem(devenv))goto devenv_ok;
                   devenv=Str(vs_installs[i].path).tailSlash(true)+"Common7\\IDE\\VCExpress.exe"; if(FExistSystem(devenv))goto devenv_ok; // express
               continue;
            devenv_ok:
               build_log=build_path+"build_log.txt"; FDelFile(build_log);
               if(build_process.create(devenv, VSBuildParams(build_project_file, config, platform, build_log)))goto build_ok;
            }
            Error("Compiling Web requires Visual C++ 2010");
         build_ok:;
         }else
         {
            build_msbuild=false;
            Str msbuild; if(devenv_version.x>=10 && build_exe_type!=EXE_WEB){msbuild=MSBuildPath(vs_path, devenv_version); if(!FExistSystem(msbuild))msbuild.clear();} // detect MSBuild for VS 2010 projects or newer (MSBuild does not support VS 2008 projects and Web)
            if( msbuild.is())
            {
               Str params=MSBuildParams(build_project_file, config, platform);
               if(build_exe_type==EXE_UWP)params.space()+="/p:AppxPackageSigningEnabled=false"; // disable code signing for Windows Universal builds, otherwise build will fail
               build_msbuild=build_process.create(msbuild, params);
            }
            if(!build_msbuild)
            {
               if(!devenv_com){build_log=build_path+"build_log.txt"; FDelFile(build_log);} // if we have "devenv.com" then we get the output from the console (devenv.exe does not generate any output)
               VSBuild(build_project_file, config, platform, build_log);
            }
         }
      }else
      if(build_exe_type==EXE_LINUX)
      {
         build_phases=1;
         build_steps =3; FREPA(build_files)if(build_files[i].mode==BuildFile::SOURCE)build_steps++; // stdafx.cpp, linking, wait for end, *.cpp

         if(!build_process.create("make", LinuxBuildParams(build_path, build_debug ? "Debug" : "Release", build_threads)))Error("Error launching \"make\" system command.\nPlease make sure you have C++ building available.");
      }else
      if(build_exe_type==EXE_MAC || build_exe_type==EXE_IOS)
      {
         build_phases=1;
         build_steps =3; FREPA(build_files)if(build_files[i].mode==BuildFile::SOURCE)build_steps++; // stdafx.cpp, linking, wait for end, *.cpp

         if(!build_process.create("xcodebuild", XcodeBuildParams(build_project_file, build_debug ? "Debug" : "Release", (build_exe_type==EXE_MAC) ? "Mac" : "iOS"))) // sdk "iphonesimulator"
            Error("Error launching \"xcodebuild\" system command.\nPlease make sure you have Xcode installed.");
      }else
      if(build_exe_type==EXE_APK)
      {
         Bool sign=!build_debug; // we want to sign with our own certificate (note that this can be done only in RELEASE mode, because in DEBUG, Android SDK will automatically sign the package with its own Debug certificate, and our own signing will fail)
         if(  sign)
         {
            // certificate file, certificate password, JDK
            if(!android_cert_file.is() || !FExistSystem(android_cert_file)                  ){options.activateCert (); Error("Android Certificate File was not specified or was not found."); return;}
            if( android_cert_pass.length()<6                                                ){options.activateCert (); Error("Android Certificate Password must be at least 6 characters long."); return;}
         #if WINDOWS
            if(!jdk_path.is() || !FExistSystem(jdk_path.tailSlash(true)+"bin/jarsigner.exe")){options.activatePaths(); Error("Path to Java Development Kit was not specified or is invalid."); return;}
         #endif
         }
         if(build_mode==BUILD_PLAY || build_mode==BUILD_DEBUG)Run(adbPath(), "start-server", true); // if we're going to launch the app then make sure that the ADB server is running, or else custom launched ADB processess will never exit
         build_steps =0;
         build_phases=1+1+sign+(build_debug ? 0 : 1)+((build_mode==BUILD_PLAY || build_mode==BUILD_DEBUG) ? 2 : 0); // ndk-build (compile) + ant (link) + jarsigner + zipalign (in release) + adb (force_stop) + adb (install)
       //if(1){build_log=build_path+"build_log.txt"; FDelFile(build_log);} this causes some freeze during building?
         build_package=AndroidPackage(cei().appPackage());
         build_process.create(ndkBuildPath(), S+"-j"+build_threads+" -C \""+build_path+"Android\""+(build_log.is() ? S+" > \""+build_log+'"' : S));

         // delete the libs so later if they exist then we can conclude that the build was successfull #AndroidArchitecture
       //FDelFile(build_path+"Android/libs/armeabi/libProject.so");
         FDelFile(build_path+"Android/libs/armeabi-v7a/libProject.so");
         FDelFile(build_path+"Android/libs/arm64-v8a/libProject.so");
         FDelFile(build_path+"Android/libs/x86/libProject.so");
      }
      buildClear ();
      buildUpdate();
   }
}
/******************************************************************************/
void CodeEditor::play()
{
   build(BUILD_PLAY);
}
/******************************************************************************/
void CodeEditor::debug()
{
 //build(BUILD_DEBUG);
   switch(config_exe)
   {
      case EXE_EXE:
      case EXE_UWP:
      case EXE_NS :
         if(Export(EXPORT_VS, BUILD_DEBUG))VSRun(build_project_file, S); break;

      case EXE_DLL: build(); break;

      default: play(); break;
   }
}
/******************************************************************************/
void CodeEditor::openIDE()
{
#if WINDOWS
                            if(Export(EXPORT_VS            , BUILD_IDE))VSOpen(build_project_file);
#elif MAC
                            if(Export(EXPORT_XCODE         , BUILD_IDE))Run   (build_project_file);
#elif LINUX
   if(verifyLinuxNetBeans())if(Export(EXPORT_LINUX_NETBEANS, BUILD_IDE))Run   (Str(netbeans_path).tailSlash(true)+"bin/netbeans", S+"--open \""+build_path+'"');
#endif
}
/******************************************************************************/
void CodeEditor::runToCursor()
{
   // TODO: support this
}
/******************************************************************************/
void CodeEditor::clean()
{
   stopBuild();
   Str build_path, build_project_name; if(getBuildPath(build_path, build_project_name))
   {
   #if MAC
      if(config_exe==EXE_MAC || config_exe==EXE_IOS)
      {
         Str project_file=build_path+"Project.xcodeproj";
         if(FExistSystem(project_file))
            REPD(debug, 2)
               if(build_process.create("xcodebuild", XcodeBuildCleanParams(project_file, debug ? "Debug" : "Release", (config_exe==EXE_MAC) ? "Mac" : "iOS"))) // sdk "iphonesimulator"
                  build_process.wait();
      }
   #endif
      FDelDirs(build_path); // delete build path
   }
}
void CodeEditor::cleanAll()
{
   clean(); // clean active project to clean it via IDE command
   FDelDirs(projects_build_path); // delete projects build path
}
/******************************************************************************/
void CodeEditor::rebuild()
{
   clean();
   rebuildSymbols(false);
   build();
}
/******************************************************************************/
void CodeEditor::configDebug(Bool debug)
{
   T.config_debug=debug;
   cei().configChangedDebug();
}
void CodeEditor::config32Bit(Bool bit32)
{
   T.config_32_bit=bit32;
   cei().configChanged32Bit();
}
void CodeEditor::configAPI(Byte api)
{
   T.config_api=api;
   cei().configChangedAPI();
}
void CodeEditor::configEXE(EXE_TYPE exe)
{
   T.config_exe=exe;
   cei().configChangedEXE();
}
/******************************************************************************/
}}
/******************************************************************************/
