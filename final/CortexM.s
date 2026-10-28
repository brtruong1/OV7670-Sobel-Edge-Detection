        THUMB
        AREA    |.text|, CODE, READONLY, ALIGN=2
        EXPORT  DisableInterrupts
        EXPORT  EnableInterrupts
        EXPORT  StartCritical
        EXPORT  EndCritical
        EXPORT  WaitForInterrupt

DisableInterrupts
        CPSID   I
        BX      LR

EnableInterrupts
        CPSIE   I
        BX      LR

StartCritical
        MRS     R0, PRIMASK
        CPSID   I
        BX      LR

EndCritical
        MSR     PRIMASK, R0
        BX      LR

WaitForInterrupt
        WFI
        BX      LR

        END