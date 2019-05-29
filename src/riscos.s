; RiscOS.s

; Veneers for RiscOS system calls
; (K) All Rites Reversed -- Copy What You Like
;
; Authors:
;       Peter Hartley       <pdh@utter.chaos.org.uk>
;
; History:
;       01-Mar-97 pdh Swiped from ExtrasLib into InterGif
;       10-Mar-97 pdh Add DrawFile, OS_Module stuff
;

        AREA |ASM$$code|, CODE, READONLY

;----------------------------
;   Declare lots of registers

R0 RN 0
R1 RN 1
R2 RN 2
R3 RN 3
R4 RN 4
R5 RN 5
R6 RN 6
R7 RN 7
R8 RN 8
R9 RN 9
R10 RN 10
R11 RN 11
R12 RN 12
R13 RN 13
R14 RN 14
PC  RN 15

;---------------------------
;   Declare some SWI numbers

XOS_Module                      * &2001E
XOS_SpriteOp                    * &2002E
XTaskWindow_TaskInfo            * &63380
XDrawFile_Render                * &65540
XDrawFile_BBox                  * &65541


;--------------------------------------------------------------------
;       void Sprite_RemoveLeftHandWastage( spritearea pArea, sprite spr )

        EXPORT Sprite_RemoveLeftHandWastage
Sprite_RemoveLeftHandWastage
        STMFD   R13!,{R14}
        MOV     R2,R1
        MOV     R1,R0
        MOV     R0,#512
        ADD     R0,R0,#54
        SWI     XOS_SpriteOp
        LDMFD   R13!,{PC}


;----------------------------------------------------------------------
;       os_error *DrawFile__BBox( int flags, const void *data, int size,
;                                 draw_matrix *dm, draw_box *result )

        EXPORT DrawFile__BBox
DrawFile__BBox
        STMFD   R13!,{R4,R14}
        LDR     R4,[R13,#8]         ; arg5 is on the stack
        SWI     XDrawFile_BBox
        MOVVC   R0,#0
        LDMFD   R13!,{R4,PC}


;------------------------------------------------------------------------
;       os_error *DrawFile__Render( int flags, const void *data, int size,
;                                   draw_matrix *dm, draw_box *result )

        EXPORT DrawFile__Render
DrawFile__Render
        STMFD   R13!,{R4,R14}
        LDR     R4,[R13,#8]         ; arg5 is on the stack
        SWI     XDrawFile_Render
        MOVVC   R0,#0
        LDMFD   R13!,{R4,PC}


;------------------------------------------------------
;       BOOL OSModule_Present( const char *modulename )

        EXPORT OSModule_Present
OSModule_Present
        STMFD   R13!,{R4,R5,R14}
        MOV     R1,R0
        MOV     R0,#18
        SWI     XOS_Module
        MOVVC   R0,#1
        MOVVS   R0,#0
        LDMFD   R13!,{R4,R5,PC}


;------------------------------------------------------
;       os_error *OSModule_Load( const char *filename )

        EXPORT OSModule_Load
OSModule_Load
        STMFD   R13!,{R14}
        MOV     R1,R0
        MOV     R0,#1
        SWI     XOS_Module
        MOVVC   R0,#0
        LDMFD   R13!,{PC}


;-------------------------------------------
;       int TaskWindow__TaskInfo( int which )

        EXPORT TaskWindow__TaskInfo
TaskWindow__TaskInfo
        STMFD   R13!,{R14}
        SWI     XTaskWindow_TaskInfo
        MOVVS   R0,#0
        LDMFD   R13!,{PC}

;
        END
