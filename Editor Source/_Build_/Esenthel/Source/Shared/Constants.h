/******************************************************************************/
extern const Str  ClientServerString ;
extern const cchar8       *WorldVerSuffix     ,
                   *ProjectsBuildPath  , // !! this will     get deleted in 'CleanAll' !!
                   *ProjectsPublishPath, // !! this will     get deleted in 'CleanAll' !!
                   *ProjectsCodePath   , // !! this will NOT get deleted in 'CleanAll' !!
                   *EsenthelProjectExt , 
                   *CodeExt            , // cpp was selected, because unlike other ideas: "code", "txt" only "cpp" gets correct coloring when opened in Visual Studio
                   *CodeSyncExt        ;
extern const Str           NullName      ,
                    UnknownName   ,
                    MultipleName  ,
                    EsenthelStoreURL;
extern const flt           StateFadeTime,
                    SkelSlotSize,
                    HeightmapTexScale,
                    WaypointRadius,
                    FlushWaypointsDelay,
                    FlushWaterDelay,
                    BuyFullVersionTime, // 10 min
                    AutoSaveTime, // 5 min
                    MaxGameViewRange    ,
                    MaxGameViewRangeDemo,
                    PropElmNameWidth, // value width of property having element name
                    SendAreasDelay,
                    VtxDupPosEps, 
                    DefaultFOV,
                    PreviewFOV;
extern const bool          MiscOnTop,
                    CodeMenuOnTop,
                    ModeTabsAlwaysVisible,
                    RequireAllCodeMatchForSync,
                    TolerantSecondaryServer, // will ignore DeviceID when getting confirmation from secondary authentication server
                    SupportBC7        , // if support BC7 compression
                    WebBC7           , // if support BC7 compression for Web TODO: enable this once browsers start supporting BC7
                    ImportRemovedElms, 
                    RenameAnimBonesOnSkelChange;
extern const COMPRESS_TYPE ServerNetworkCompression     , ClientNetworkCompression     , EsenthelProjectCompression     ;
extern const cchar8       *      AppName,
                   *ServerAppName,
                   *InstallerName;
extern const cchar8       *ObjAccessNames[]
;
extern int ObjAccessNamesElms;
extern const cchar8 *ElmNameMesh,
             *ElmNameSkel,
             *ElmNamePhys;
extern const    ImagePtr    ImageNull;
extern const MaterialPtr MaterialNull;
extern bool          IsServer;
extern ReadWriteSync WorldAreaSync;
/******************************************************************************/
Str SDKPath();
Str BinPath();
bool LoadOK(LOAD_RESULT result);
bool CanRead     (USER_ACCESS access);
bool CanWrite    (USER_ACCESS access);
bool CanWriteCode(USER_ACCESS access);
TimeStamp Min(C TimeStamp &a, C TimeStamp &b);
TimeStamp Max(C TimeStamp &a, C TimeStamp &b);
/******************************************************************************/
