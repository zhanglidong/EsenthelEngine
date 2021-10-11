/******************************************************************************

   Uploader:
      Uses:
         1               IO       Thread
         N             * Compress Threads (N depends on available RAM memory)
         connections() * FTP      Threads

      IO -> Compressor -> FTP

      Uses lower case naming system for all files on the server (because of linux)
      Prefixes files with "f-" and directories with "d-" (to avoid name collisions with files/directories), 'ServerPath' is used for that

      Downloads Index (if any):
         PAK Compressed Encrypted
            *.pak

      Downloads Installer Info (if any):
         TextData Uncompressed NotEncrypted
            "Size"
            "xxHash32"
            "xxHash64"
            "ModifyTimeUTC"
            "Version"

      Creates Directories

      Uploads Files
         Tries to compress them first (if compressed size is smaller, then PakFile.compression is marked with compressed)
         Files after compression are uploaded (with optional encryption)
         Files are uploaded with "@new" suffix
         File Data: Raw -> Calculate xxHash64_32 -> Compress -> Encrypt

      Uploads Installers (original installer name, name+".installer."+Patcher.Platform())
         EXE Uncompressed NotEncrypted

      Uploads Installers Info (name+".installer."+Patcher.Platform()+".txt")
         TextData Uncompressed NotEncrypted
            "Size"
            "xxHash32"
            "xxHash64"
            "ModifyTimeUTC"
            "Version"
            "Name" = original installer name

      Uploads New Pak Index + PHP Update Script

      PHP Script:
         Renames all files by removing their "@new" suffix
         Removes all obsolete files
         Removes all empty    directories

/******************************************************************************/
enum IO_CMD
{
   IO_NONE      ,
   IO_CREATE_PAK,
   IO_HAVE_PAK  ,
   IO_LOAD      ,
}
enum FTP_CMD
{
   FTP_NONE          ,
   FTP_DOWNLOAD_FILES,
   FTP_HAVE_INDEX    ,
   FTP_CREATE_DIRS   ,
   FTP_CREATED_DIRS  ,
   FTP_UPLOAD_FILES  ,
   FTP_UPLOAD_INDEX  ,
   FTP_LOG_OUT       ,
}
/******************************************************************************/
const Str  new_suffix="@new";
const bool ignore_auth_result=true;
const int    InstallerPlatforms=4;
const OS_VER InstallerPlatform[]=
{
   WINDOWS_UNKNOWN,
   OS_MAC,
   OS_LINUX,
   OS_ANDROID,
};
const COMPRESS_TYPE Compression=COMPRESS_LZMA;
const int           CompressionLevel=9;
/******************************************************************************/
Str ServerPath(Str path, bool file=true) // !! this needs to be in sync with 'Patcher' class !!
{
   Str out; if(file){Str ext=GetExt(path); if(ext=="pl" || ext=="php" || ext=="old" || ext=="dat" || ext=="dll" || ext=="bat" || ext=="cmd")path+='_';} // these file formats can't be downloaded normally, because they're treated as scripts or some can be blocked by servers
   for(;;)
   {
      Str base=GetBase(path); if(!base.is())break;
      out =S + (file ? "f-" : "d-") + CaseDown(base) + (out.is() ? '/' : '\0') + out;
      path=GetPath(path);
      file=false;
   }
   return out;
}
/******************************************************************************/
class InstallerInfo
{
   long     size;
   uint     xxHash32;
   ulong    xxHash64;
   DateTime modify_time_utc;
   VecI4    ver;
   Str      name;

   InstallerInfo() {reset();}
   void    reset() {size=0; xxHash32=0; xxHash64=0; modify_time_utc.zero(); ver.zero(); name.clear();}

   static bool SameHash(uint  a, uint  b) {return a==b || !a || !b;}
   static bool SameHash(ulong a, ulong b) {return a==b || !a || !b;}

   bool operator==(InstallerInfo &ii) {return size==ii.size && SameHash(xxHash32, ii.xxHash32) && SameHash(xxHash64, ii.xxHash64) && !Compare(modify_time_utc, ii.modify_time_utc, 1) && ver==ii.ver && Equal(name, ii.name, true);}
   bool operator!=(InstallerInfo &ii) {return !(T==ii);}

   bool hasUnicode()C {return HasUnicode(name);}
   void save(TextData &data)
   {
      data.nodes.New().set("Size"         , size);
      data.nodes.New().set("xxHash32"     , TextHex(xxHash32,  8, 0, false));
      data.nodes.New().set("xxHash64"     , TextHex(xxHash64, 16, 0, false));
      data.nodes.New().set("ModifyTimeUTC", modify_time_utc.asText(true));
      if(Compare(ver, VecI4(0))>0)data.nodes.New().set("Version", TextVer(ver));
      data.nodes.New().set("Name"         , name);
   }
   void load(TextData &data)
   {
      reset();
      if(TextNode *p=data.findNode("Size"         ))size    =p.asLong();
      if(TextNode *p=data.findNode("xxHash32"     ))xxHash32=TextUInt (S+"0x"+p.value);
      if(TextNode *p=data.findNode("xxHash64"     ))xxHash64=TextULong(S+"0x"+p.value);
      if(TextNode *p=data.findNode("ModifyTimeUTC"))modify_time_utc.fromText (p.value);
      if(TextNode *p=data.findNode("Version"      ))ver=TextVer(p.value);
      if(TextNode *p=data.findNode("Name"         ))name=GetBase(p.value); // use 'GetBase' for safety in case server data got hijacked, because we're going to delete that file
   }
   void load(C Str &name)
   {
      reset();
      FileInfo fi; if(fi.get       (name)){modify_time_utc=fi.modify_time_utc; size=fi.size; T.name=GetBase(name);} // set params only if file exists
      File     f ; if(f .readStdTry(name)){f.pos(0); xxHash32=f.xxHash32(); f.pos(0); xxHash64=f.xxHash64();}
      ver=FileVersion(name); if(Compare(ver, VecI4(0))<0)ver.zero(); // if ver<0 then reset to 0
   }
}
class LoadFile
{
   enum COMPRESS_MODE : byte
   {
      COMPRESS_NO,
      COMPRESS_RAW,
      COMPRESS_FULL,
   }

   bool          encrypt=true; 
   COMPRESS_MODE compress=COMPRESS_RAW;
   long          uncompressed_size=0; // uncompressed size used to calculate the upload progress
   Str           src_path, ftp_path;
 C PakFile      *pf=null; // needed for updating header data based on compression results

   void set(C Str &src_path, C Str &ftp_path, C PakFile *pf, long uncompressed_size=0, bool encrypt=true, COMPRESS_MODE compress=COMPRESS_RAW) {T.src_path=src_path; T.ftp_path=ftp_path; T.pf=pf; T.uncompressed_size=uncompressed_size; T.encrypt=encrypt; T.compress=compress;}
}
class Loaded : LoadFile
{
   Mems<byte> data;

   void create(LoadFile &load, File &src) {SCAST(LoadFile, T)=load; src.get(data.setNum(src.size()).data(), src.size());}

   int memUsage() {return data.elms();}
}
class Down
{
   enum TYPE : byte
   {
      NONE,
      INDEX,
      INSTALLER,
   }

   bool success   =false,
        encrypt   =true,
        decompress=true;
   TYPE type;
   byte sub_type;
   Str  display,
        path;
   File data;

   void set(TYPE type, C Str &display, C Str &path, bool encrypt, bool decompress, byte sub_type=0) {T.type=type; T.display=display; T.path=path; T.encrypt=encrypt; T.decompress=decompress; T.sub_type=sub_type;}
}
class Upload
{
   bool encrypt=true;
   Str  display,
        path;
   File data;
   long uncompressed_size=0; // uncompressed size used to calculate the upload progress

   int memUsage() {return data.size();}

   void createIndex(Pak &pak, C Str &path)
   {
      T.path=path;
      T.display="Index";
      File temp; temp.writeMem(); pak.saveHeader(temp);
      data.writeMem(); temp.pos(0); Compress(temp, data, Compression, CompressionLevel, false);
   }

   void createText(FileText &f, C Str &path, C Str &display)
   {
      T.encrypt=false;
      T.path=path;
      T.display=display;
      f.rewind().copy(T.data.writeMem());
   }
   void createText(C Str &text, C Str &path, C Str &display)
   {
      FileText f; f.writeMem(HasUnicode(text) ? UTF_8 : ANSI); f.putText(text); // it's important to use ANSI when there's no Unicode, because HTAccess will not work if there's a BOM
      createText(f, path, display);
   }

   void create(Loaded &src)
   {
      encrypt=src.encrypt;
      display=GetBase(src.src_path);
      path   =src.ftp_path;
      uncompressed_size=src.uncompressed_size;

      // calculate xxHash64_32
      if(src.pf)ConstCast(src.pf.data_xxHash64_32)=xxHash64_32Mem(src.data.data(), src.data.elms());

      // compress file
      File temp; temp.readMem(src.data.data(), src.data.elms());
      data.writeMem(src.data.elms()/10);
      switch(src.compress)
      {
         case LoadFile.COMPRESS_FULL:
         {
            if(Compressable(GetExt(src.src_path)) // compression can potentially decrease the file size
            && Compress(temp, data, Compression, CompressionLevel, UP.cmpr_threads.elms()<=1) // compressed ok (use multi-threaded compression only if using up to 1 compression threads)
            && data.size()<temp.size()) // compressed size is smaller
            {
               if(src.pf) // update PAK header
               {
                  ConstCast(src.pf.compression         )=Compression; // set compression algorithm
                  ConstCast(src.pf.data_size_compressed)=data.size(); // set compressed  size
               }
               return;
            }
            temp.pos(0); Compress(temp, data.reset(), COMPRESS_NONE); // store inside 'Compress' container, but using no compression, needed because decompression always uses 'Decompress'
         }return;

         case LoadFile.COMPRESS_RAW:
         {
            if(Compressable(GetExt(src.src_path)) // compression can potentially decrease the file size
            && CompressRaw(temp, data, Compression, CompressionLevel, UP.cmpr_threads.elms()<=1) // compressed ok (use multi-threaded compression only if using up to 1 compression threads)
            && data.size()<temp.size()) // compressed size is smaller
            {
               if(src.pf) // update PAK header
               {
                  ConstCast(src.pf.compression         )=Compression; // set compression algorithm
                  ConstCast(src.pf.data_size_compressed)=data.size(); // set compressed  size
               }
               return;
            }
            temp.pos(0); // use uncompressed data as below
         }break;
      }
      temp.copy(data.reset()); // use uncompressed data
   }
}
/******************************************************************************/
class FTP
{
   bool      set_text=false;
   Str       text;
   Ftp       ftp;
   Thread    thread;
   Uploader *uploader=null;
   Progress  progress;
   Text     tprogress;
   long      uncompressed_size=0;

   static bool FtpThread(Thread &thread) {((FTP*)thread.user).update(); return true;}

   void del() {thread.del();} // delete the thread first
  ~FTP() {del();}

   dbl uploaded()C {return progress()*uncompressed_size;}

   void    setText    (C Str &text ) {SyncLocker locker(uploader.ftp_lock); T.set_text=true; T.text=text;}
   void updateProgress(            ) {SyncLocker locker(uploader.ftp_lock); if(set_text){set_text=false; tprogress.set(text);} int total=ftp.total(); progress.set((total>=0) ? flt(ftp.progress())/total : 0);}
   void  resetProgress(bool visible)
   {
       progress.clear().visible(visible);
      tprogress.clear().visible(visible);
   }

   void finished() // !! warning this may get called on a secondary thread and not 'update' !!
   {
       progress.hide();
      tprogress.hide();
      uncompressed_size=0;
   }

   void update()
   {
      SyncLockerEx locker(uploader.ftp_lock);

      Cipher *cipher; switch(uploader.cipher())
      {
         default: cipher=             null; break;
         case  1: cipher=&uploader.cipher1; break;
         case  2: cipher=&uploader.cipher2; break;
         case  3: cipher=&uploader.cipher3; break;
      }
      Str host=uploader.ftp_host(), user=uploader.ftp_user(), pass=uploader.ftp_pass();
      switch(  uploader.ftp_cmd)
      {
         case FTP_DOWNLOAD_FILES:
         {
            if(!ftp.is()){setText("Logging In"); locker.off(); if(!ftp.logIn(host, user, pass, ignore_auth_result)){Gui.msgBox("Error", "Can't login to Ftp"); uploader.force_stop=true;}}
            if( ftp.is())
            {
               locker.on();
               if(!uploader.ftp_download_files.elms()){setText(S); locker.off(); Time.wait(1);}else
               {
                  Down down; Swap(down, uploader.ftp_download_files.last()); uploader.ftp_download_files.removeLast(); setText(S+"Downloading \""+down.display+"\""); locker.off();
                  File temp;
                  REPD(passive, 2) // start from passive
                     if(ftp.download(down.path, temp.writeMem(), 0, passive!=0, down.encrypt ? cipher : null))
                  {
                     temp.pos(0);
                     if(down.decompress)
                     {
                        if(Decompress(temp, down.data.writeMem()))down.success=true;
                     }else
                     {
                        if(temp.copy(down.data.writeMem()))down.success=true;
                     }
                     down.data.pos(0);
                     break; // no need for another "passive" download
                  }
                  locker.on(); setText(S); Swap(down, uploader.ftp_downloaded_files.New());
               }
            }
         }break;

         case FTP_CREATE_DIRS:
         {
            if(this!=&uploader.ftp[0]){locker.off(); Time.wait(1);}else // create dirs only on single thread (so they are created in order)
            {
               int dirs=uploader.ftp_create_dirs.elms();
               if(!dirs)uploader.ftp_cmd=FTP_CREATED_DIRS;else
               {
                  Str name=uploader.ftp_create_dirs.last(); uploader.ftp_create_dirs.removeLast(); setText(S+"Creating Folders ("+dirs+" left)"); locker.off();
                  if(!ftp.is())if(!ftp.logIn(host, user, pass, ignore_auth_result)){Gui.msgBox("Error", "Can't login to Ftp"); uploader.force_stop=true;}
                  ftp.createDir(name);
               }
            }
         }break;

         case FTP_UPLOAD_FILES:
         case FTP_UPLOAD_INDEX:
         {
            if(!ftp.is()){setText("Logging In"); locker.off(); if(!ftp.logIn(host, user, pass, ignore_auth_result)){setText("Login Failed"); Time.wait(1000);}}
            if( ftp.is())
            {
               locker.on();
               if(!uploader.ftp_upload_files.elms()){setText("Waiting for file to upload"); locker.off(); Time.wait(1);}else
               {
                  Upload up; Swap(up, uploader.ftp_upload_files.last()); uploader.ftp_upload_files.removeLast();
                  setText(S+"Uploading \""+up.display+"\""); uncompressed_size=up.uncompressed_size;
                  locker.off();
                  up.data.pos(0); if(ftp.upload(up.data, up.path, true, up.encrypt ? cipher : null)){locker.on(); uploader.ftp_files_to_upload--; uploader.ftp_size_uploaded+=uncompressed_size; uncompressed_size=0;}else // start from passive
                  {
                     up.data.pos(0); if(ftp.upload(up.data, up.path, false, up.encrypt ? cipher : null)){locker.on(); uploader.ftp_files_to_upload--; uploader.ftp_size_uploaded+=uncompressed_size; uncompressed_size=0;}else
                     {
                        locker.on(); Swap(up, uploader.ftp_upload_files.New()); uncompressed_size=0; locker.off(); // put back on the list
                        setText("Upload failed, trying again"); ftp.logOut(); Time.wait(250);
                     }
                  }
               }
            }
         }break;

         case FTP_LOG_OUT:
         {
            if(ftp.is()){setText("Logging out"); locker.off(); ftp.logOut();}
            setText(S); Time.wait(1);
         }break;

         default: locker.off(); Time.wait(1); break;
      }
   }

   void threadCreate() {if(!thread.active())thread.create(FtpThread, this, 0, false, "FTP");}
}
/******************************************************************************/
class Uploader
{
   TextStyle ts, ts_small;
   Text     tftp_host, tftp_user, tftp_pass, tftp_name, tftp_dir, thttp_dir, tsrc, tinstaller[InstallerPlatforms],             tftp_conn, tupdate;
   TextLine  ftp_host,  ftp_user,  ftp_pass,  ftp_name,  ftp_dir,  http_dir,  src,  installer[InstallerPlatforms], cipher_key                    ;
   ComboBox                                                                                                        cipher    ,  ftp_conn         ;
   Button    upload, stop, bupdate;

   bool force_stop=false, uploading=false;
   int  call=0; // gets incremented with each stop/start, to make sure that data processed on secondary threads is passed on further only for same upload calls, this is because secondary threads (IO+COMPRESSION) are not stopped when upload is stopped (they keep going), which could result their output be passed on to the next upload

   Str  index_path, installer_dcmp_path[InstallerPlatforms], installer_cmpr_path[InstallerPlatforms], installer_info_path[InstallerPlatforms];
   Pak  pak_src, pak_dest;
   bool server_pak_loaded_ok=false;

   Cipher1 cipher1;
   Cipher2 cipher2;
   Cipher3 cipher3;

   FTP          ftp[8];
   SyncLock     ftp_lock;
   FTP_CMD      ftp_cmd;
   Memc<Down  > ftp_download_files, ftp_downloaded_files;
   Memc<Str   > ftp_create_dirs;
   Memc<Upload> ftp_upload_files;
   int          ftp_files_to_upload=0;
   long         ftp_size_to_upload=0, ftp_size_uploaded=0; // this is before compression, because we do not know the total compressed size until all files have been compressed

   Thread         io_thread;
   SyncLock       io_lock;
   IO_CMD         io_cmd;
   Memc<LoadFile> io_load_files;

   SyncLock                    cmpr_lock;
   Memc<Loaded>                cmpr_loaded_files;
   Mems<const_mem_addr Thread> cmpr_threads;
   
   Memc<Str> rename_files, remove_files, remove_dirs;

   int connections()C {return ftp_conn()+1;}

   Uploader() {REPAO(ftp).uploader=this;}
  ~Uploader() {uploadStop(); io_thread.del(); cmpr_threads.del(); REPAO(ftp).del();}

   static bool IOThread(Thread &thread)
   {
      Uploader &uploader=*(Uploader*)thread.user;
      SyncLockerEx locker(uploader.io_lock);
      switch(uploader.io_cmd)
      {
         case IO_CREATE_PAK:
         {
            Str name=uploader.src(); int call=uploader.call; locker.off();
            Pak pak_src; pak_src.create(name, S, PAK_SHORTEN|PAK_NO_DATA|PAK_NO_FILE);
            locker.on(); if(call==uploader.call){Swap(pak_src, uploader.pak_src); uploader.io_cmd=IO_HAVE_PAK;} // pass on further only if we're still in the same call
         }break;

         case IO_LOAD:
         {
            if(!uploader.io_load_files.elms() || uploader.highMemUsage()){locker.off(); Time.wait(1);}else
            {
               LoadFile load; Swap(load, uploader.io_load_files.last()); uploader.io_load_files.removeLast(); int call=uploader.call; locker.off();
               File     src ; if(!src.readStdTry(load.src_path)){Gui.msgBox("Error", S+"Can't open file:\n\""+load.src_path+"\""); uploader.force_stop=true;}else
               {
                  Loaded loaded; loaded.create(load, src);
                  SyncLocker locker_cmpr(uploader.cmpr_lock); if(call==uploader.call)Swap(uploader.cmpr_loaded_files.New(), loaded); // pass on further only if we're still in the same call
               }
            }
         }break;

         default: locker.off(); Time.wait(1); break;
      }
      return true;
   }
   static bool CompressThread(Thread &thread)
   {
      Uploader &uploader=*(Uploader*)thread.user;
      SyncLockerEx locker(uploader.cmpr_lock);
      if(!uploader.cmpr_loaded_files.elms()){locker.off(); Time.wait(1);}else
      {
         Loaded loaded; Swap(loaded, uploader.cmpr_loaded_files.last()); uploader.cmpr_loaded_files.removeLast(); int call=uploader.call; locker.off();
         Upload upload; upload.create(loaded); SyncLocker locker_ftp(uploader.ftp_lock); if(call==uploader.call)Swap(uploader.ftp_upload_files.New(), upload); // pass on further only if we're still in the same call
      }
      return true;
   }

   static void Start (Uploader &uploader) {uploader.uploadStart();}
   static void Stop  (Uploader &uploader) {uploader.uploadStop ();}
   static void Update(Uploader &uploader) {uploader.updateDo   ();}

   bool highMemUsage()
   {
      SyncLocker locker_ftp(ftp_lock), locker_io(io_lock), locker_cmpr(cmpr_lock);
      long size=0;
      FREPA( ftp_upload_files)size+= ftp_upload_files[i].memUsage();
      FREPA(cmpr_loaded_files)size+=cmpr_loaded_files[i].memUsage();
      return size>1*1024*1024*1024; // 1 GB
   }

   Uploader& disabled(bool disabled)
   {
       uploading=disabled;
        ftp_host.disabled( disabled);
        ftp_user.disabled( disabled);
        ftp_pass.disabled( disabled);
        ftp_name.disabled( disabled);
        ftp_dir .disabled( disabled);
       http_dir .disabled( disabled);
        src     .disabled( disabled);
REPAO(installer).disabled( disabled);
      cipher    .disabled( disabled);
      cipher_key.disabled( disabled);
        ftp_conn.disabled( disabled);
          upload.visible (!disabled);
            stop.visible ( disabled);
         bupdate.visible ( false   );
         tupdate.visible ( false   );
      REPAO(ftp).resetProgress(disabled && InRange(i, connections()));
      return T;
   }

   void uploadStart()
   {
      uploadStop();

      App.stayAwake(AWAKE_SYSTEM);

      // verify settings
      FileInfo fi; bool ok;
                                              ok=false; if(fi.get(src         ()))if(fi.type==FSTD_DIR )ok=true; if(!ok){Gui.msgBox("Error", S+"Folder \""   +src         ()+"\"\n not found"); return;}
      FREPA(installer)if(installer[i]().is()){ok=false; if(fi.get(installer[i]()))if(fi.type==FSTD_FILE)ok=true; if(!ok){Gui.msgBox("Error", S+"Installer \""+installer[i]()+"\"\n not found"); return;}}
      if(cipher())
      {
         Memc<Str > keys=Split(cipher_key(), ','); if(keys.elms()<=0){invalid_key: Gui.msgBox(S, "Invalid Encryption Key"); return;}
         Mems<byte> key; key.setNum(keys.elms()); CalcValue cv; REPA(key){TextValue(keys[i], cv, false); if(!cv.type)goto invalid_key; key[i]=cv.asInt();}
         cipher1.setKey(key.data(), key.elms());
         cipher2.setKey(key.data(), key.elms());
         cipher3.setKey(key.data(), key.elms());
      }

                              index_path            =Str(ftp_dir()).tailSlash(true)+CaseDown(ftp_name())+".index.pak";
      REP(InstallerPlatforms){installer_dcmp_path[i]=Str(ftp_dir()).tailSlash(true)+GetBase(installer[i]()); // use original name
                              installer_cmpr_path[i]=Str(ftp_dir()).tailSlash(true)+CaseDown(ftp_name())+".installer."+Patcher.Platform(InstallerPlatform[i]);
                              installer_info_path[i]=Str(ftp_dir()).tailSlash(true)+CaseDown(ftp_name())+".installer."+Patcher.Platform(InstallerPlatform[i])+".txt";}

      disabled(true);

      // start downloading
      {
         SyncLocker locker(ftp_lock);
         ftp_cmd=FTP_DOWNLOAD_FILES;
                                   ftp_download_files.New().set(Down.INDEX    , "Index"         , index_path            , true , true);
         FREPA(installer_info_path)ftp_download_files.New().set(Down.INSTALLER, "Installer Info", installer_info_path[i], false, false, i);
         int conn=Min(ftp_download_files.elms(), connections()); FREP(conn)ftp[i].threadCreate();
      }
      {
         SyncLocker locker(io_lock);
         io_cmd=IO_CREATE_PAK;
         pak_src.del();
         if(!io_thread.created())io_thread.create(IOThread, this, 0, false, "IO");
      }
   }
   void stopFTP()
   {
      REPAO(ftp).thread.stop();
      {SyncLocker locker(ftp_lock); ftp_cmd=FTP_NONE; REPAO(ftp).ftp.abort();}
      REPAO(ftp).thread.del();
      REPAO(ftp).ftp   .del();
   }
   void uploadStop()
   {
      call++; // this will prevent data being passed on further

      {SyncLocker locker(  io_lock); io_cmd=IO_NONE; io_load_files.clear();}
      {SyncLocker locker(cmpr_lock); cmpr_loaded_files.clear();}

      stopFTP();

      {SyncLocker locker(ftp_lock); ftp_download_files.clear(); ftp_downloaded_files.clear(); ftp_create_dirs.clear(); ftp_upload_files.clear(); ftp_files_to_upload=0; ftp_size_to_upload=ftp_size_uploaded=0;}

      disabled(false);
      rename_files.clear();
      remove_files.clear();
      remove_dirs .clear();
      pak_dest    .del  (); server_pak_loaded_ok=false;

      App.stayAwake(AWAKE_OFF);
      App.stateNormal();
   }
   void updateDo()
   {
      uploadStop();
      Str http=http_dir();
      if(!Starts(http, "http:") && !Starts(http, "https:"))http=S+"http://"+http;
      http.tailSlash(true); http+=CaseDown(ftp_name())+".update.php";
      Explore(Replace(http, '\\', '/'));
   }

   static cchar8 *conns  []={"1", "2", "3", "4", "5", "6", "7", "8"};
   static cchar8 *encrypt[]={"No Encryption", "Cipher 1", "Cipher 2", "Cipher 3"};
   
   static void CipherChanged(Uploader &uploader) {uploader.cipher_key.visible(uploader.cipher()!=0);}

   Uploader& create()
   {
      Flt x0=-D.w()+0.02, x1=D.w()-0.02, w=D.w()*1.45, y=D.h()-0.05, h=0.06, s=0.065;
      ts.reset(true).size=0.055; ts.align.set(1, 0);
      Gui+=tftp_host.create(Vec2(x0, y), "Ftp Host Name"   , &ts); Gui+=ftp_host .create(Rect_R(x1, y, w, h), "ftp.domain.com"     ).desc("Ftp Host Name\nFor example: ftp.domain.com\nor: ftps://ftp.domain.com"); y-=s;
      Gui+=tftp_user.create(Vec2(x0, y), "Ftp User Name"   , &ts); Gui+=ftp_user .create(Rect_R(x1, y, w, h), "user"               ).desc("Ftp User Name\nFor example: user"          ); y-=s;
      Gui+=tftp_pass.create(Vec2(x0, y), "Ftp Password"    , &ts); Gui+=ftp_pass .create(Rect_R(x1, y, w, h), "password"           ).desc("Ftp Password\nFor example: password"       ); y-=s;
      Gui+=tftp_name.create(Vec2(x0, y), "Ftp Dest Name"   , &ts); Gui+=ftp_name .create(Rect_R(x1, y, w, h), "GameName"           ).desc("Destination name of the upload\nFor example: GameName"); y-=s;
      Gui+=tftp_dir .create(Vec2(x0, y), "Ftp Dest Folder" , &ts); Gui+=ftp_dir  .create(Rect_R(x1, y, w, h), "www/download"       ).desc("Destination folder location on the Ftp where to upload files\nFor example: www/download"); y-=s;
      Gui+=thttp_dir.create(Vec2(x0, y), "Http Dest Folder", &ts); Gui+=http_dir .create(Rect_R(x1, y, w, h), "domain.com/download").desc("Destination folder location for Http where files will be uploaded\nThis must match the Ftp Dest Folder location (but in Http format)\nFor example: domain.com/download"); y-=s;
      Gui+=tsrc     .create(Vec2(x0, y), "Src Folder"      , &ts); Gui+=src      .create(Rect_R(x1, y, w, h), "C:\\Game"           ).desc("Source folder on your hard drive that will have its contents uploaded\nFor example: C:\\Game"); y-=s;
      FREPA(installer)
      {
         Gui+=tinstaller[i].create(Vec2(x0, y), S+"Installer "+OSName(InstallerPlatform[i]), &ts); Gui+=installer[i].create(Rect_R(x1, y, w, h), S).desc("Latest version of installer on your hard drive that will be uploaded\nFor example: C:\\Game Installer.exe\n\nYour custom patcher should always try to update itself by downloading the latest\ninstaller version, before downloading the file data."); y-=s;
      }
      Gui+= cipher  .create(Rect_L(x0, y, x1-w-x0, h), encrypt, Elms(encrypt)).func(CipherChanged, T).set(0); Gui+=cipher_key.create(Rect_R(x1, y, w, h)).desc("256 number Encryption key, in \"x, x, x, x, x..\" format"); y-=s;
      Gui+=tftp_conn.create(Vec2(x0, y), "Ftp Connections" , &ts); Gui+=ftp_conn .create(Rect_R(x1, y, w, h), conns, Elms(conns)).set(3).desc("Number of Ftp connections"); y-=s;
      Gui+=   upload.create(Rect_C (0, Avg(y,  -D.h()     ), 0.7 , h*1.7), "Upload").func(Start , T);
      Gui+=     stop.create(Rect_RD(D.w(),     -D.h()      , 0.17, h    ), "Stop"  ).func(Stop  , T);
      Gui+=  bupdate.create(Rect_C (0, Avg (y, -D.h()     ), 0.4 , h*1.4), "Update").func(Update, T);
      Gui+=  tupdate.create(Rect_C (0, Lerp(y, -D.h(),0.1f), 0.7 , h    ), "Files have been uploaded,\npress \"Update\" to perform the final update.");

      flt pos=y, space=(pos-(-D.h()))/Elms(ftp), height=space*0.83;
      ts_small.reset(true); ts_small.size=height*1.2;
      FREPA(ftp){Gui+=ftp[i].progress.create(Rect_C(0, pos, 0.9, height)); Gui+=ftp[i].tprogress.create(Vec2(0, pos), S, &ts_small); pos-=space;}

      MemStats mem; mem.get();
      int threads=(mem.avail_phys-(1ll*1024*1024*1024))/CompressionMemUsage(Compression, CompressionLevel, 64*1024*1024); // leave 1 GB for OS and stuff
      cmpr_threads.setNum(Mid(threads, 1, Cpu.threads()));
      REPAO(cmpr_threads).create(CompressThread, this, 0, false, "Compress");

      return disabled(false);
   }

   void setFilesToUpload()
   {
      // get info about ServerPak and ServerInstaller
      InstallerInfo installer_server[InstallerPlatforms];
      REPA(ftp_downloaded_files)
      {
         Down      &down=ftp_downloaded_files[i];
         Mems<byte> data; data.setNum(down.data.size()); down.data.pos(0); down.data.get(data.data(), data.elms());
         switch(down.type)
         {
            case Down.INDEX:
            {
               PAK_LOAD load=pak_dest.loadMemEx(data.data(), data.elms());
               server_pak_loaded_ok=(load==PAK_LOAD_OK || load==PAK_LOAD_INCOMPLETE_DATA);
            }break;

            case Down.INSTALLER:
            {
               FileText f; f.readMem(data.data(), data.elms());
               TextData data; data.load(f); installer_server[down.sub_type].load(data);
            }break;
         }
      }

      Str ftp_path=ftp_dir(); ftp_path.tailSlash(true)+=CaseDown(ftp_name()).tailSlash(true);
      Str src_path=src    (); src_path.tailSlash(true);

      // list creating folders and uploading files
      REPA(pak_src) // go from end to add base folders at the end (because thread that creates dirs takes last elements first)
      {
       C PakFile &src =pak_src .file(i   ); Str path=pak_src.fullName(src); // local
       C PakFile *dest=pak_dest.find(path);                                 // server
         if(src.children_num) // folder
         {
            if(!dest || !dest.children_num)ftp_create_dirs.add(ftp_path+ServerPath(path, false));
         }
         if(src.data_size) // file with data
         {
            if(!dest || dest.data_size!=src.data_size || Compare(dest.modify_time_utc, src.modify_time_utc, 1)) // if different than upload
            {
               Str file_ftp_path=ftp_path+ServerPath(path)+new_suffix;
               io_load_files.New().set(src_path+path, file_ftp_path, &src, src.data_size);
                rename_files.add(file_ftp_path);
            }else // if equal
            if(dest) // retain parameters from server version
            {
               ConstCast(src.compression         )=dest.compression;
               ConstCast(src.data_xxHash64_32    )=dest.data_xxHash64_32;
               ConstCast(src.data_size_compressed)=dest.data_size_compressed;
               ConstCast(src.flag                )=dest.flag;
            }
         }
      }
      for(Str path=ftp_path; path.is(); path=GetPath(path))ftp_create_dirs.add(path); // create ftp folders, starting from "ftp_dir/name"

      // list files for removal
      REPA(pak_dest) // server files
      {
       C PakFile &dest=pak_dest.file(i   ); Str path=pak_dest.fullName(dest); // server
       C PakFile *src =pak_src .find(path);                                   // local
         if(dest.children_num) // folder
         {
            if(!src || !src.children_num)remove_dirs.add(ftp_path+ServerPath(path, false));
         }
         if(dest.data_size) // file with data
         {
            if(!src || src.data_size==0)remove_files.add(ftp_path+ServerPath(path));
         }
      }

      // installer
      FREP(InstallerPlatforms)
      {
         InstallerInfo installer_local; installer_local.load(installer[i]());
         if(installer_local!=installer_server[i]) // local version is different than server
         {
            if(installer[i]().is()) // upload
            {
               Str file_ftp_path;

               // installer info
               TextData data; installer_local.save(data); FileText f; f.writeMem(installer_local.hasUnicode() ? UTF_8 : ANSI); data.save(f);
               file_ftp_path=installer_info_path[i]+new_suffix;
               ftp_upload_files.New().createText(f, file_ftp_path, "Installer Info"); rename_files.add(file_ftp_path);

               // installer compressed
               file_ftp_path=installer_cmpr_path[i]+new_suffix;
               io_load_files.New().set(installer[i](), file_ftp_path, null, installer_local.size, true , LoadFile.COMPRESS_FULL); rename_files.add(file_ftp_path);

               // installer decompressed
               file_ftp_path=installer_dcmp_path[i]+new_suffix;
               io_load_files.New().set(installer[i](), file_ftp_path, null, installer_local.size, false, LoadFile.COMPRESS_NO  ); rename_files.add(file_ftp_path);
               
               if(InstallerPlatform[i]==OS_ANDROID && Compare(installer_local.ver, installer_server[i].ver)<=0)Gui.msgBox("Warning", "Android installer file is different, but its Build Version is not newer - Android OS may refuse to update the app.");
            }else // remove
            {
               remove_files.add(installer_info_path[i]); // del installer info
               remove_files.add(installer_cmpr_path[i]); // del installer compressed (path is always the same)
            }
            if(       installer_server[i].name.is()
            && !Equal(installer_server[i].name, installer_local.name, true))
               remove_files.add(Str(ftp_dir()).tailSlash(true)+installer_server[i].name); // del installer decompressed (with old original name) if old exists and is different than new (this covers both cases when #1 new exists but different and #2 new is removed), this requires 'remove_files' to be executed first for case insensitive servers
         }else
         if(!server_pak_loaded_ok) // or server pak failed to load (which means that perhaps we're using a different cipher now, in which case we have to reupload compressed installers)
         {
            if(installer[i]().is()) // upload
            {
               Str file_ftp_path=installer_cmpr_path[i]+new_suffix;
               io_load_files.New().set(installer[i](), file_ftp_path, null, installer_local.size, true, LoadFile.COMPRESS_FULL); rename_files.add(file_ftp_path);
            }
         }
      }

      // PHP Download Script
      /*{
         Str file_ftp_path=Str(ftp_path).tailSlash(false)+".download.php"+new_suffix;
         ftp_upload_files.New().createText(PHPDownloadScript(), file_ftp_path, "PHP Download Script"); rename_files.add(file_ftp_path);
      }*/

      // HT Access
      {
         Str file_ftp_path=ftp_dir(); file_ftp_path.tailSlash(true)+=S+".htaccess"+new_suffix;
         ftp_upload_files.New().createText(HTAccess(), file_ftp_path, "htaccess"); rename_files.add(file_ftp_path);
      }

      // get total files to upload, set at the end, after setting all files
      ftp_files_to_upload=io_load_files.elms()+ftp_upload_files.elms();
      ftp_size_to_upload=ftp_size_uploaded=0; REPA(io_load_files)ftp_size_to_upload+=io_load_files[i].uncompressed_size;

      ftp_cmd=(ftp_create_dirs.elms() ? FTP_CREATE_DIRS : FTP_CREATED_DIRS);
       io_cmd=IO_LOAD;
   }

   /*Str PHPDownloadScript()
   {
      Str s;
      s+="<?php\r\n";
      s+="$file=$_GET['file'];\r\n";
      s+="$file=str_replace('\\\\','/',$file); // use unix style paths\r\n";
      s+="if($file[0]!='/' && $file[1]!=':' && strpos($file,'../')===false && strpos($file,'//')===false) // does not start with root/drive and does not contain relative paths or urls like http://\r\n";
      s+="{\r\n";
      s+="\t$file=basename(__FILE__,'.download.php').'/'.$file;\r\n";
      s+="\tif(is_readable($file))\r\n";
      s+="\t{\r\n";
      s+="\t\theader('Content-Description: File Transfer');\r\n";
      s+="\t\theader('Content-Type: application/octet-stream');\r\n";
      s+="\t\theader('Content-Transfer-Encoding: binary');\r\n";
      s+="\t\theader('Expires: 0');\r\n";
      s+="\t\theader('Cache-Control: must-revalidate, no-transform');\r\n";
      s+="\t\theader('Pragma: public');\r\n";
      s+="\t\theader('Content-Length: '.filesize($file));\r\n";
      s+="\t\tob_clean();\r\n";
      s+="\t\tflush();\r\n";
      s+="\t\treadfile($file);\r\n";
      s+="\t\texit;\r\n";
      s+="\t}\r\n";
      s+="}\r\n";
      s+="?>\r\n";
      return s;
   }*/
   Str PHPUpdateScript()
   {
      Str s;
      s+="<?php\r\n";
      s+="function ren($src, $dest) {if(!rename($src, $dest))echo(\"Error renaming \".$src.\" file.<br/>\");}\r\n";
      s+="function exist  ($name) {return file_exists($name);}\r\n";
      s+="function delFile($name) {if(exist($name))unlink($name);}\r\n";
      s+="function delDir ($name) {if(exist($name))rmdir ($name);}\r\n";
      FREPA(remove_files) // !! first remove old files, so folders will be empty for removal AND new installer is not accidentally deleted for case insensitive platforms !! (new installer problem is: old installer name "installer", new installer name "Installer", if we insert new first, it will rename "Installer@new" to "Installer" and then trying to delete the old "installer" on case insensitive servers would delete the new, because of that delete first)
      {
         Str name=Replace(SkipStartPath(remove_files[i], ftp_dir()), '\\', '/');
         s+=S+"delFile(\""+name+"\");\r\n";
      }
      FREPA(rename_files) // now rename
      {
         Str src=Replace(SkipStartPath(rename_files[i], ftp_dir()), '\\', '/'), dest=SkipEnd(src, new_suffix);
         s+=S+"ren(\""+src+"\", \""+dest+"\");\r\n";
      }
      FREPA(remove_dirs) // now remove empty folders
      {
         Str name=Replace(SkipStartPath(remove_dirs[i], ftp_dir()), '\\', '/');
         s+=S+"delDir(\""+name+"\");\r\n";
      }
      s+=S+"unlink(\""+CaseDown(GetBase(ftp_name()))+".update.php\");\r\n"; // remove PHP script
      s+="?>\r\n";
      s+="Update Finished!";
      return s;
   }
   Str HTAccess()
   {
      Str s;
      s+="Header append Cache-Control \"must-revalidate, no-transform\"\r\nHeader append Vary \"Accept-Encoding\"\r\n";
      return s;
   }

   // update
   void update()
   {
      if(force_stop)
      {
         uploadStop();
         force_stop=false;
      }else
      {
         SyncLockerEx locker_io(io_lock), locker_ftp(ftp_lock);
         REPAO(ftp).updateProgress();
         if(io_cmd==IO_HAVE_PAK && ftp_cmd==FTP_DOWNLOAD_FILES && ftp_downloaded_files.elms()==1+InstallerPlatforms) // index+installers
         {
            setFilesToUpload();
         }
         if(ftp_cmd==FTP_CREATED_DIRS)
         {
            ftp_cmd=FTP_UPLOAD_FILES;
            FREP(connections())ftp[i].threadCreate();
         }
         if(ftp_cmd==FTP_UPLOAD_FILES)
         {
            if(ftp_files_to_upload<=0)
            {
               ftp_files_to_upload=0;
               ftp_cmd=FTP_UPLOAD_INDEX;
               ftp_files_to_upload++; ftp_upload_files.New().createIndex(pak_src, index_path+new_suffix); rename_files.add(index_path+new_suffix); // add as last after all other uploads finished, because their compression might change the Index Header, but before the PHP update script
               ftp_files_to_upload++; ftp_upload_files.New().createText (PHPUpdateScript(), Str(ftp_dir()).tailSlash(true)+CaseDown(ftp_name())+".update.php", "PHP Update Script"); // add after index
            }
         }
         if(ftp_cmd==FTP_UPLOAD_INDEX)
         {
            if(ftp_files_to_upload<=0) // finished
            {
               App.stateNormal().flash();
               locker_ftp.off();
               stopFTP(); REPAO(ftp).finished();
               tupdate.visible(true);
               bupdate.visible(true);
               App.stayAwake(AWAKE_OFF);
            }
         }
         if(ftp_cmd)
         {
            if(ftp_size_to_upload)
            {
               dbl uploaded=ftp_size_uploaded; REPA(ftp)uploaded+=ftp[i].uploaded();
               App.stateProgress(uploaded/ftp_size_to_upload);
            }
         }
      }
   }

   // io
   void save(C Str &name=GetExtNot(App.exe())+".txt")
   {
      XmlData xml;
      XmlNode &node=xml.nodes.New().setName("Config");
      node.params.New().set("FtpHostName"   ,  ftp_host ());
      node.params.New().set("FtpUserName"   ,  ftp_user ());
      node.params.New().set("FtpPassword"   ,  ftp_pass ());
      node.params.New().set("FtpDestName"   ,  ftp_name ());
      node.params.New().set("FtpDestFolder" ,  ftp_dir  ());
      node.params.New().set("HttpDestFolder", http_dir  ());
      node.params.New().set("SrcFolder"     ,  src      ());
      FREPA(installer)node.params.New().set(Replace(tinstaller[i](), ' ', '\0'), installer[i]());
      node.params.New().set("Cipher"        , cipher    ());
      node.params.New().set("CipherKey"     , cipher_key());
      node.params.New().set("FtpConnections", connections());
      xml.save(name, true);
   }

   void load(C Str &name=GetExtNot(App.exe())+".txt")
   {
      XmlData xml; if(xml.load(name))if(XmlNode *node=xml.findNode("config"))
      {
         if(XmlParam *p=node.findParam("FtpHostName"   ))ftp_host  .set(p.value);
         if(XmlParam *p=node.findParam("FtpUserName"   ))ftp_user  .set(p.value);
         if(XmlParam *p=node.findParam("FtpPassword"   ))ftp_pass  .set(p.value);
         if(XmlParam *p=node.findParam("FtpDestName"   ))ftp_name  .set(p.value);
         if(XmlParam *p=node.findParam("FtpDestFolder" ))ftp_dir   .set(p.value);
         if(XmlParam *p=node.findParam("HttpDestFolder"))http_dir  .set(p.value);
         if(XmlParam *p=node.findParam("SrcFolder"     ))src       .set(p.value);
         if(XmlParam *p=node.findParam("Cipher"        ))cipher    .set(p.asInt());
         if(XmlParam *p=node.findParam("CipherKey"     ))cipher_key.set(p.value);
         if(XmlParam *p=node.findParam("FtpConnections"))ftp_conn  .set(Mid(TextInt(p.value), 1, ftp_conn.menu.list.elms())-1);
         if(XmlParam *p=node.findParam("Installer"     )){OS_VER ver=OSGroup(); REPA(InstallerPlatform)if(InstallerPlatform[i]==ver){installer[i].set(p.value); break;}} // deprecated
         FREPA(installer)if(XmlParam *p=node.findParam(Replace(tinstaller[i](), ' ', '\0')))installer[i].set(p.value);
      }
   }
}
Uploader UP;
/******************************************************************************/
void InitPre()
{
   EE_INIT(false, false);
   App.flag=APP_WORK_IN_BACKGROUND|APP_MINIMIZABLE|APP_NO_PAUSE_ON_WINDOW_MOVE_SIZE;
   App.x=1; App.y=-1;
   flt scale=D.screenH()/1080.0;
   D.mode(512*scale, 400*scale).scale(1.66).shadowMapSize(0);
#if DEBUG
   App.flag|=APP_MEM_LEAKS|APP_BREAKPOINT_ON_ERROR;
   Paks.add(EE_ENGINE_PATH);
#else
   if(!EE_ENGINE_EMBED)Paks.add("Engine.pak");
#endif
   ASSERT(Elms(InstallerPlatform)==InstallerPlatforms);
}
Bool Init()
{
   UP.create().load();
   return true;
}
/******************************************************************************/
void Shut()
{
   UP.save();
}
/******************************************************************************/
bool Update()
{
   if(!UP.uploading && Kb.bp(KB_ESC))return false;
   if(!App.active())Time.wait(30);

   // move window
   if(Ms.b(0))
   {
      GUI_OBJ_TYPE type=(Gui.ms() ? Gui.ms().type() : GO_NONE);
      if(type!=GO_BUTTON && type!=GO_TEXTLINE && type!=GO_CHECKBOX)App.window().move(Ms.pixelDelta().x, Ms.pixelDelta().y);
   }

   UP .update();
   Gui.update();
   return true;
}
/******************************************************************************/
void Draw()
{
   D.clear(ColorBrightness(0.85));
   Gui.draw();
}
/******************************************************************************/
