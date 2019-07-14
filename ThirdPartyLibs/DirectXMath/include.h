/******************************************************************************/
#if !WINDOWS
   #define _In_range_(a, b)
   #define _In_
   #define _In_z_
   #define _In_opt_
   #define _Out_
   #define _Inout_
   #define _Analysis_assume_(x)
   #define _Out_opt_
   #define _Out_writes_(x)
   #define _Out_writes_opt_(x)
   #define _Out_writes_all_(x)
   #define _Out_writes_bytes_(x)
   #define _In_reads_(x)
   #define _In_reads_bytes_(x)
   #define _In_reads_opt_(x)
   #define _Inout_updates_all_(x)
   #define _Inout_updates_all_opt_(x)
   #define _Inout_updates_bytes_all_(x)
   #define _Use_decl_annotations_
   #define _When_(x, y)
   #define _Success_(x)
   #define XM_CALLCONV
   #define _XM_NO_INTRINSICS_
   #define __cdecl
   typedef UInt DWORD;
   typedef Int HRESULT;
   #define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
   #define E_INVALIDARG                     _HRESULT_TYPEDEF_(0x80070057L)
   #define E_OUTOFMEMORY                    _HRESULT_TYPEDEF_(0x8007000EL)
   #define E_UNEXPECTED                     _HRESULT_TYPEDEF_(0x8000FFFFL)
   #define E_FAIL                           _HRESULT_TYPEDEF_(0x80004005L)
   #define E_POINTER                        _HRESULT_TYPEDEF_(0x80004003L)
   #define S_OK                            ((HRESULT)0L)
   #define ERROR_ARITHMETIC_OVERFLOW 534L
   #define FAILED(hr) (((HRESULT)(hr)) < 0)
   #define FACILITY_WIN32 7
   inline HRESULT HRESULT_FROM_WIN32(unsigned long x) { return (HRESULT)(x) <= 0 ? (HRESULT)(x) : (HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000);}
   #include "../DirectX/dxgiformat.h"
#endif

#if WINDOWS // WINDOWS already has these headers included always, and including our custom ones would introduce conflict, so in this case always use system headers
   #include <DirectXMath.h>
   #include <DirectXPackedVector.h>
#else
   #include "DirectXMath.h"
   #include "DirectXPackedVector.h"
#endif
/******************************************************************************/
