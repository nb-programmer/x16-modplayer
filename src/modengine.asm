
.pc02

;MOD player data sharing interface
.struct ModEngineData
    write_buffer .addr
    read_buffer .addr
    status .byte
    ticks_per_row .byte
    current_row_tick .byte
    sample_state .addr
.endstruct
.enum status_flag
    writing_buf_idx = %00000001
    enable_playback = %00000010
.endenum

.struct SampleState
	spl_ptr .addr
	spl_bank .byte
	spl_position .word
	play_pitch .byte
	control .byte
.endstruct

.include "cx16.inc"
.include "x16kernal.inc"
.include "macros.inc"

AMIGA_SAMPLE_RATE = SAMPLERATE(8000)
AMIGA_TICKS_PER_ROW = 30 ;Correct value is 6 ticks/row for 125BPM

;Make sure it is a multiple of 256 ($100)
AUDIO_BUFFER_SIZE = $100

.zeropage

.exportzp _modengine
;IRQ setup variables
irq_default: .addr 0
vera_ien_default: .byte 0

;Sound engine
_modengine: .tag ModEngineData

.bss
;Calculation buffer for audio
sndbuf: .res AUDIO_BUFFER_SIZE

.code

.export _modengine_loop
.proc _modengine_loop
    wai
    bra _modengine_loop
    rts
.endproc

;Just trying out something, the counter is going to overflow and cause noise
.proc audio_sync_buffer
    ;Copy a block of data to the FIFO buffer, using an unrolled loop
    ldy #0
    .repeat $100
        lda (_modengine + ModEngineData::read_buffer), y
        sta VERA::PCM::DATA
        iny
    .endrepeat
    ;Increase the address high byte
    inc _modengine + ModEngineData::read_buffer + 1
    rts
.endproc

;TODO
.proc modengine_render_next_row
    rts
.endproc

.proc mod_irq_handler
    lda VERA::IRQ_FLAGS
    and #VERA::VERT_SYNC
    beq :+

    ;[VSYNC Interrupt]
    ;Clear IRQ flag
    lda #VERA::VERT_SYNC
    sta VERA::IRQ_FLAGS

    bbr1 _modengine + ModEngineData::status, @no_row_update
    inc _modengine + ModEngineData::current_row_tick

    lda _modengine + ModEngineData::current_row_tick
    cmp _modengine + ModEngineData::ticks_per_row
    bne @no_row_update

    stz _modengine + ModEngineData::current_row_tick
    jsr modengine_render_next_row
@no_row_update:
:
    lda VERA::IRQ_FLAGS
    and #VERA::AUDIO_LOW
    beq :+
    ;[AUDIO LOW interrupt]
    ;Can only end if the buffer is at least 1/4 full
    jsr audio_sync_buffer
:
    jmp (irq_default)
.endproc

.export _modengine_init
.proc _modengine_init
    ;Stop the PCM engine
    stz VERA::PCM::RATE

    lda #AMIGA_TICKS_PER_ROW
    sta _modengine + ModEngineData::ticks_per_row

    jsr _modengine_initbuffer

    ;Set-up custom interrupt handler
    jsr initISR

    ;Reset the PCM FIFO, set volume to max, and use stereo 8-bit audio
    lda #(VERA::PCM::RESET | $0F) ;| VERA::PCM::STEREO
    sta VERA::PCM::CTRL

    ;Fill the FIFO to not immediately get an IRQ
    jsr audio_clear_fifo

    rts
.endproc

.export _modengine_free
.proc _modengine_free
    jsr _modengine_disable_audio
    jsr freeISR
    rts
.endproc

.export _modengine_initbuffer
.proc _modengine_initbuffer
    lda #<sndbuf
    sta _modengine + ModEngineData::write_buffer
    sta _modengine + ModEngineData::read_buffer
    lda #>sndbuf
    sta _modengine + ModEngineData::write_buffer + 1
    sta _modengine + ModEngineData::read_buffer + 1

    rts
.endproc

.export _modengine_enable_audio
.proc _modengine_enable_audio
    ;Enable VERA interrupts
    sei
    lda VERA::IRQ_EN
    sta vera_ien_default
    ora #(VERA::VERT_SYNC | VERA::AUDIO_LOW)
    ;sta VERA::IRQ_EN
    
    ;Enable the playback flag
    smb1 _modengine + ModEngineData::status

    cli

    ;Set sample rate >0 to enable PCM playback
    lda #AMIGA_SAMPLE_RATE
    sta VERA::PCM::RATE

    rts
.endproc


.export _modengine_disable_audio
.proc _modengine_disable_audio
    ;Disable VERA interrupts
    sei
    ;Disable the playback flag
    rmb1 _modengine + ModEngineData::status

    ;Disable the interrupts we had enabled
    lda vera_ien_default
    sta VERA::IRQ_EN
    cli

    ;Set sample rate =0 to disable PCM playback
    stz VERA::PCM::RATE

    rts
.endproc


;Simply store 0s to the PCM FIFO to fill it up completely
;Call this to fill the audio queue with silence
.proc audio_clear_fifo
    ldx #0
@putlp:
.repeat 16
    stz VERA::PCM::DATA
.endrepeat
    dex
    bne @putlp
    bit VERA::PCM::CTRL
    bpl @putlp
    rts
.endproc

;Sets up a custom interrupt handler (ISR)
.proc initISR
    lda IRQVec
    sta irq_default
    lda IRQVec+1
    sta irq_default+1
    sei
    lda #<mod_irq_handler
    sta IRQVec
    lda #>mod_irq_handler
    sta IRQVec+1
    cli
    rts
.endproc

;Cleanup custom ISR
.proc freeISR
    sei
    lda irq_default
    sta IRQVec
    lda irq_default+1
    sta IRQVec+1
    cli
    rts
.endproc

