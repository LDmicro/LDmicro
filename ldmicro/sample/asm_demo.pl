   0:# INIT TABLES
   1:# INIT VARS
   2:if not '$once_0_INIT_VARS' {
   3:    set bit '$once_0_INIT_VARS'
   4:    let var 'TUpdateOF' := 49
   5:}
  25:# 
  26:# ======= START RUNG 1 =======
  28:LabelRung1: // 
  29:# Try compile HEX->ASM.
  30:# 
  31:# 
  32:# ======= START RUNG 2 =======
  34:LabelRung2: // 
  35:# 
  36:# 1) You can change 'saved' in EEPROM with PERSITS via UART RECV.
  37:# 
  38:# ======= START RUNG 3 =======
  40:LabelRung3: // 
  41:set bit '$rung_top'
  43:# start series [
  44:# ELEM_PERSIST
  45:if '$rung_top' {
  46:    if not '$once_1_PERSIST_saved' {
  47:        clear bit '$scratch'
  48:        set bit '$scratch' if EEPROM busy
  49:        if not '$scratch' {
  50:            set bit '$once_1_PERSIST_saved'
  51:            read EEPROM[0,0+1] into 'saved'
  52:        }
  53:    } else {
  54:        clear bit '$scratch'
  55:        set bit '$scratch' if EEPROM busy
  56:        if not '$scratch' {
  57:            read EEPROM[0,0+1] into '$tmpVar24bit'
  58:            if '$tmpVar24bit' == 'saved' {
  59:            } else {
  60:                write 'saved' into EEPROM[0,0+1]
  61:            }
  62:        }
  63:    }
  64:}
  66:# ] finish series
  67:# 
  68:# ======= START RUNG 4 =======
  70:LabelRung4: // 
  71:set bit '$rung_top'
  73:# start series [
  74:# ELEM_ONE_SHOT_RISING
  75:if '$rung_top' {
  76:    if '$once_2_ONE_SHOT_RISING_' {
  77:        clear bit '$rung_top'
  78:    } else {
  79:        set bit '$once_2_ONE_SHOT_RISING_'
  80:    }
  81:} else {
  82:    clear bit '$once_2_ONE_SHOT_RISING_'
  83:}
  85:# start parallel [
  86:clear bit '$parOut_0'
  87:let bit '$parThis_0' := '$rung_top'
  88:# ELEM_EQU
  89:if '$parThis_0' {
  90:    if 'saved' != '0' {
  91:        clear bit '$parThis_0'
  92:    }
  93:}
  95:if '$parThis_0' {
  96:    set bit '$parOut_0'
  97:}
  98:let bit '$parThis_0' := '$rung_top'
  99:# ELEM_EQU
 100:if '$parThis_0' {
 101:    if 'saved' != '0xffff' {
 102:        clear bit '$parThis_0'
 103:    }
 104:}
 106:if '$parThis_0' {
 107:    set bit '$parOut_0'
 108:}
 109:let bit '$rung_top' := '$parOut_0'
 110:# ] finish parallel
 111:# ELEM_MOVE
 112:if '$rung_top' {
 113:    let var 'saved' := 49
 114:}
 116:# ] finish series
 117:# 
 118:# ======= START RUNG 5 =======
 120:LabelRung5: // 
 121:set bit '$rung_top'
 123:# start series [
 124:# ELEM_UART_RECV
 125:if '$rung_top' {
 126:    uart recv int 'saved', have? into '$rung_top'
 127:}
 129:# ] finish series
 130:# 
 131:# ======= START RUNG 6 =======
 133:LabelRung6: // 
 134:# 
 135:# 2) Press input Contact and see output Coil.
 136:# 
 137:# ======= START RUNG 7 =======
 139:LabelRung7: // 
 140:set bit '$rung_top'
 142:# start series [
 143:# start parallel [
 144:clear bit '$parOut_1'
 145:let bit '$parThis_1' := '$rung_top'
 146:# ELEM_CONTACTS
 147:if 'Ycycle2' {
 148:    clear bit '$parThis_1'
 149:}
 151:if '$parThis_1' {
 152:    set bit '$parOut_1'
 153:}
 154:let bit '$parThis_1' := '$rung_top'
 155:# ELEM_CONTACTS
 156:if not 'Xled' {
 157:    clear bit '$parThis_1'
 158:}
 160:if '$parThis_1' {
 161:    set bit '$parOut_1'
 162:}
 163:let bit '$rung_top' := '$parOut_1'
 164:# ] finish parallel
 165:# ELEM_COIL
 166:let bit 'Ycycle2' := '$rung_top'
 168:# ] finish series
 169:# 
 170:# ======= START RUNG 8 =======
 172:LabelRung8: // 
 173:# 3) ADC input changes PWM duty on output.
 174:# Memo: PWM sets only once after reset.
 175:# 
 176:# ======= START RUNG 9 =======
 178:LabelRung9: // 
 179:set bit '$rung_top'
 181:# start series [
 182:# ELEM_READ_ADC
 183:if '$rung_top' {
 184:    read adc 'ADCvalue'
 185:}
 187:# ] finish series
 188:# 
 189:# ======= START RUNG 10 =======
 191:LabelRung10: // 
 192:set bit '$rung_top'
 194:# start series [
 195:# ] finish series
 196:# 
 197:# ======= START RUNG 11 =======
 199:LabelRung11: // 
 200:set bit '$rung_top'
 202:# start series [
 203:# ELEM_DIV
 204:if '$rung_top' {
 205:    let var '$scratch2' := 16
 206:    let var 'PWMduty' := 'ADCvalue' / '$scratch2'
 207:}
 209:# ] finish series
 210:# 
 211:# ======= START RUNG 12 =======
 213:LabelRung12: // 
 214:set bit '$rung_top'
 216:# start series [
 217:# ELEM_SET_PWM
 218:if '$rung_top' {
 219:    set pwm 'PWMduty' % 1000 Hz out 'P'
 220:    set bit '$P'
 221:}
 223:# ] finish series
 224:# 
 225:# ======= START RUNG 13 =======
 227:LabelRung13: // 
 228:# 
 229:# 4) Set output Coils and print information to UART.
 230:# 
 231:# ======= START RUNG 14 =======
 233:LabelRung14: // 
 234:set bit '$rung_top'
 236:# start series [
 237:# ELEM_CONTACTS
 238:if 'RUpdate' {
 239:    clear bit '$rung_top'
 240:}
 242:# ELEM_TON
 243:if '$rung_top' {
 244:    if 'TUpdateON' < '49' {
 245:        clear bit '$rung_top'
 246:        increment 'TUpdateON'
 247:    }
 248:} else {
 249:    let var 'TUpdateON' := 0
 250:}
 252:# ELEM_TOF
 253:if not '$rung_top' {
 254:    if 'TUpdateOF' < '49' {
 255:        increment 'TUpdateOF'
 256:        set bit '$rung_top'
 257:    }
 258:} else {
 259:    let var 'TUpdateOF' := 0
 260:}
 262:# start parallel [
 263:let bit '$parThis_2' := '$rung_top'
 264:# ELEM_COIL
 265:let bit 'RUpdate' := '$parThis_2'
 267:let bit '$parThis_2' := '$rung_top'
 268:# ELEM_COIL
 269:let bit 'YUpdate' := '$parThis_2'
 271:# ] finish parallel
 272:# ] finish series
 273:# 
 274:# ======= START RUNG 15 =======
 276:LabelRung15: // 
 277:set bit '$rung_top'
 279:# start series [
 280:# ELEM_CONTACTS
 281:if not 'RUpdate' {
 282:    clear bit '$rung_top'
 283:}
 285:# ELEM_ONE_SHOT_RISING
 286:if '$rung_top' {
 287:    if '$once_3_ONE_SHOT_RISING_' {
 288:        clear bit '$rung_top'
 289:    } else {
 290:        set bit '$once_3_ONE_SHOT_RISING_'
 291:    }
 292:} else {
 293:    clear bit '$once_3_ONE_SHOT_RISING_'
 294:}
 296:# start parallel [
 297:let bit '$parThis_3' := '$rung_top'
 298:# start series [
 299:# ELEM_GEQ
 300:if '$parThis_3' {
 301:    if 'ADCvalue' < '700' {
 302:        clear bit '$parThis_3'
 303:    }
 304:}
 306:# ELEM_COIL
 307:if '$parThis_3' {
 308:    set bit 'YTEMP_HI'
 309:}
 311:# ] finish series
 312:let bit '$parThis_3' := '$rung_top'
 313:# start series [
 314:# ELEM_LEQ
 315:if '$parThis_3' {
 316:    if 'ADCvalue' > '650' {
 317:        clear bit '$parThis_3'
 318:    }
 319:}
 321:# ELEM_COIL
 322:if '$parThis_3' {
 323:    clear bit 'YTEMP_HI'
 324:}
 326:# ] finish series
 327:let bit '$parThis_3' := '$rung_top'
 328:# start series [
 329:# ELEM_EQU
 330:if '$parThis_3' {
 331:    if 'display' != '0' {
 332:        clear bit '$parThis_3'
 333:    }
 334:}
 336:# ELEM_FORMATTED_STRING
 337:if '$parThis_3' {
 338:    if not '$once_4_FMTD_STR_' {
 339:        set bit '$once_4_FMTD_STR_'
 340:        let var '$fmtd_0_seq' := 0
 341:        set bit '$fmtd_3_doSend'
 342:    }
 343:} else {
 344:    clear bit '$once_4_FMTD_STR_'
 345:}
 346:let var '$seqScratch' := '$fmtd_0_seq'
 347:if '$fmtd_0_seq' < '13' {
 348:} else {
 349:    let var '$seqScratch' := -1
 350:}
 351:if '$fmtd_3_doSend' {
 352:    clear bit '$scratch'
 353:    '$scratch' = is uart ready to send ?
 354:    if not '$scratch' {
 355:        let var '$seqScratch' := -1
 356:    }
 357:}
 358:let var '$scratch' := 0
 359:if '$scratch' == '$seqScratch' {
 360:    let var '$charToUart' := 65
 361:}
 362:let var '$scratch' := 1
 363:if '$scratch' == '$seqScratch' {
 364:    let var '$charToUart' := 68
 365:}
 366:let var '$scratch' := 2
 367:if '$scratch' == '$seqScratch' {
 368:    let var '$charToUart' := 67
 369:}
 370:let var '$scratch' := 3
 371:if '$scratch' == '$seqScratch' {
 372:    let var '$charToUart' := 118
 373:}
 374:let var '$scratch' := 4
 375:if '$scratch' == '$seqScratch' {
 376:    let var '$charToUart' := 97
 377:}
 378:let var '$scratch' := 5
 379:if '$scratch' == '$seqScratch' {
 380:    let var '$charToUart' := 108
 381:}
 382:let var '$scratch' := 6
 383:if '$scratch' == '$seqScratch' {
 384:    let var '$charToUart' := 117
 385:}
 386:let var '$scratch' := 7
 387:if '$scratch' == '$seqScratch' {
 388:    let var '$charToUart' := 101
 389:}
 390:let var '$scratch' := 8
 391:if '$scratch' == '$seqScratch' {
 392:    let var '$charToUart' := 61
 393:}
 394:let var '$scratch' := 9
 395:clear bit '$scratch'
 396:if '$scratch' == '$seqScratch' {
 397:    set bit '$scratch'
 398:}
 399:if '$scratch' {
 400:    let var '$fmtd_1_convertState' := 'ADCvalue'
 401:    set bit '$fmtd_2_isLeadingZero'
 402:    let var '$scratch' := 1000
 403:    let var '$charToUart' := '$fmtd_1_convertState' / '$scratch'
 404:    let var '$scratch' := '$scratch' * '$charToUart'
 405:    let var '$fmtd_1_convertState' := '$fmtd_1_convertState' - '$scratch'
 406:    let var '$scratch' := 48
 407:    let var '$charToUart' := '$charToUart' + '$scratch'
 408:    if '$scratch' == '$charToUart' {
 409:        if '$fmtd_2_isLeadingZero' {
 410:            let var '$charToUart' := 32
 411:        }
 412:    } else {
 413:        clear bit '$fmtd_2_isLeadingZero'
 414:    }
 415:}
 416:let var '$scratch' := 10
 417:clear bit '$scratch'
 418:if '$scratch' == '$seqScratch' {
 419:    set bit '$scratch'
 420:}
 421:if '$scratch' {
 422:    let var '$scratch' := 100
 423:    let var '$charToUart' := '$fmtd_1_convertState' / '$scratch'
 424:    let var '$scratch' := '$scratch' * '$charToUart'
 425:    let var '$fmtd_1_convertState' := '$fmtd_1_convertState' - '$scratch'
 426:    let var '$scratch' := 48
 427:    let var '$charToUart' := '$charToUart' + '$scratch'
 428:    if '$scratch' == '$charToUart' {
 429:        if '$fmtd_2_isLeadingZero' {
 430:            let var '$charToUart' := 32
 431:        }
 432:    } else {
 433:        clear bit '$fmtd_2_isLeadingZero'
 434:    }
 435:}
 436:let var '$scratch' := 11
 437:clear bit '$scratch'
 438:if '$scratch' == '$seqScratch' {
 439:    set bit '$scratch'
 440:}
 441:if '$scratch' {
 442:    let var '$scratch' := 10
 443:    let var '$charToUart' := '$fmtd_1_convertState' / '$scratch'
 444:    let var '$scratch' := '$scratch' * '$charToUart'
 445:    let var '$fmtd_1_convertState' := '$fmtd_1_convertState' - '$scratch'
 446:    let var '$scratch' := 48
 447:    let var '$charToUart' := '$charToUart' + '$scratch'
 448:    if '$scratch' == '$charToUart' {
 449:        if '$fmtd_2_isLeadingZero' {
 450:            let var '$charToUart' := 32
 451:        }
 452:    } else {
 453:        clear bit '$fmtd_2_isLeadingZero'
 454:    }
 455:}
 456:let var '$scratch' := 12
 457:clear bit '$scratch'
 458:if '$scratch' == '$seqScratch' {
 459:    set bit '$scratch'
 460:}
 461:if '$scratch' {
 462:    let var '$scratch' := 1
 463:    let var '$charToUart' := '$fmtd_1_convertState' / '$scratch'
 464:    let var '$scratch' := '$scratch' * '$charToUart'
 465:    let var '$fmtd_1_convertState' := '$fmtd_1_convertState' - '$scratch'
 466:    let var '$scratch' := 48
 467:    let var '$charToUart' := '$charToUart' + '$scratch'
 468:}
 469:if '$seqScratch' < '0' {
 470:} else {
 471:    if '$fmtd_3_doSend' {
 472:        uart send from '$charToUart'
 473:        increment '$fmtd_0_seq'
 474:    }
 475:}
 476:clear bit '$parThis_3'
 477:if '$fmtd_0_seq' < '13' {
 478:    if '$fmtd_3_doSend' {
 479:        set bit '$parThis_3'
 480:    }
 481:} else {
 482:    clear bit '$fmtd_3_doSend'
 483:}
 485:# start parallel [
 486:# ELEM_MOVE
 487:if '$parThis_3' {
 488:    let var 'display' := 1
 489:}
 491:let bit '$parThis_4' := '$parThis_3'
 492:# start series [
 493:# ELEM_ONE_SHOT_FALLING
 494:if not '$parThis_4' {
 495:    if '$once_5_ONE_SHOT_FALLING_' {
 496:        clear bit '$once_5_ONE_SHOT_FALLING_'
 497:        set bit '$parThis_4'
 498:    }
 499:} else {
 500:    set bit '$once_5_ONE_SHOT_FALLING_'
 501:    clear bit '$parThis_4'
 502:}
 504:# ELEM_MOVE
 505:if '$parThis_4' {
 506:    let var 'display' := 2
 507:}
 509:# ] finish series
 510:# ] finish parallel
 511:# ] finish series
 512:# ] finish parallel
 513:# ] finish series
 514:# 
 515:# ======= START RUNG 16 =======
 517:LabelRung16: // 
 518:set bit '$rung_top'
 520:# start series [
 521:# ELEM_EQU
 522:if '$rung_top' {
 523:    if 'display' != '2' {
 524:        clear bit '$rung_top'
 525:    }
 526:}
 528:# ELEM_FORMATTED_STRING
 529:if '$rung_top' {
 530:    if not '$once_6_FMTD_STR_' {
 531:        set bit '$once_6_FMTD_STR_'
 532:        let var '$fmtd_4_seq' := 0
 533:        set bit '$fmtd_7_doSend'
 534:    }
 535:} else {
 536:    clear bit '$once_6_FMTD_STR_'
 537:}
 538:let var '$seqScratch' := '$fmtd_4_seq'
 539:if '$fmtd_4_seq' < '13' {
 540:} else {
 541:    let var '$seqScratch' := -1
 542:}
 543:if '$fmtd_7_doSend' {
 544:    clear bit '$scratch'
 545:    '$scratch' = is uart ready to send ?
 546:    if not '$scratch' {
 547:        let var '$seqScratch' := -1
 548:    }
 549:}
 550:let var '$scratch' := 0
 551:if '$scratch' == '$seqScratch' {
 552:    let var '$charToUart' := 32
 553:}
 554:let var '$scratch' := 1
 555:if '$scratch' == '$seqScratch' {
 556:    let var '$charToUart' := 80
 557:}
 558:let var '$scratch' := 2
 559:if '$scratch' == '$seqScratch' {
 560:    let var '$charToUart' := 87
 561:}
 562:let var '$scratch' := 3
 563:if '$scratch' == '$seqScratch' {
 564:    let var '$charToUart' := 77
 565:}
 566:let var '$scratch' := 4
 567:if '$scratch' == '$seqScratch' {
 568:    let var '$charToUart' := 100
 569:}
 570:let var '$scratch' := 5
 571:if '$scratch' == '$seqScratch' {
 572:    let var '$charToUart' := 117
 573:}
 574:let var '$scratch' := 6
 575:if '$scratch' == '$seqScratch' {
 576:    let var '$charToUart' := 116
 577:}
 578:let var '$scratch' := 7
 579:if '$scratch' == '$seqScratch' {
 580:    let var '$charToUart' := 121
 581:}
 582:let var '$scratch' := 8
 583:if '$scratch' == '$seqScratch' {
 584:    let var '$charToUart' := 61
 585:}
 586:let var '$scratch' := 9
 587:clear bit '$scratch'
 588:if '$scratch' == '$seqScratch' {
 589:    set bit '$scratch'
 590:}
 591:if '$scratch' {
 592:    let var '$fmtd_5_convertState' := 'PWMduty'
 593:    set bit '$fmtd_6_isLeadingZero'
 594:    let var '$scratch' := 1000
 595:    let var '$charToUart' := '$fmtd_5_convertState' / '$scratch'
 596:    let var '$scratch' := '$scratch' * '$charToUart'
 597:    let var '$fmtd_5_convertState' := '$fmtd_5_convertState' - '$scratch'
 598:    let var '$scratch' := 48
 599:    let var '$charToUart' := '$charToUart' + '$scratch'
 600:    if '$scratch' == '$charToUart' {
 601:        if '$fmtd_6_isLeadingZero' {
 602:            let var '$charToUart' := 32
 603:        }
 604:    } else {
 605:        clear bit '$fmtd_6_isLeadingZero'
 606:    }
 607:}
 608:let var '$scratch' := 10
 609:clear bit '$scratch'
 610:if '$scratch' == '$seqScratch' {
 611:    set bit '$scratch'
 612:}
 613:if '$scratch' {
 614:    let var '$scratch' := 100
 615:    let var '$charToUart' := '$fmtd_5_convertState' / '$scratch'
 616:    let var '$scratch' := '$scratch' * '$charToUart'
 617:    let var '$fmtd_5_convertState' := '$fmtd_5_convertState' - '$scratch'
 618:    let var '$scratch' := 48
 619:    let var '$charToUart' := '$charToUart' + '$scratch'
 620:    if '$scratch' == '$charToUart' {
 621:        if '$fmtd_6_isLeadingZero' {
 622:            let var '$charToUart' := 32
 623:        }
 624:    } else {
 625:        clear bit '$fmtd_6_isLeadingZero'
 626:    }
 627:}
 628:let var '$scratch' := 11
 629:clear bit '$scratch'
 630:if '$scratch' == '$seqScratch' {
 631:    set bit '$scratch'
 632:}
 633:if '$scratch' {
 634:    let var '$scratch' := 10
 635:    let var '$charToUart' := '$fmtd_5_convertState' / '$scratch'
 636:    let var '$scratch' := '$scratch' * '$charToUart'
 637:    let var '$fmtd_5_convertState' := '$fmtd_5_convertState' - '$scratch'
 638:    let var '$scratch' := 48
 639:    let var '$charToUart' := '$charToUart' + '$scratch'
 640:    if '$scratch' == '$charToUart' {
 641:        if '$fmtd_6_isLeadingZero' {
 642:            let var '$charToUart' := 32
 643:        }
 644:    } else {
 645:        clear bit '$fmtd_6_isLeadingZero'
 646:    }
 647:}
 648:let var '$scratch' := 12
 649:clear bit '$scratch'
 650:if '$scratch' == '$seqScratch' {
 651:    set bit '$scratch'
 652:}
 653:if '$scratch' {
 654:    let var '$scratch' := 1
 655:    let var '$charToUart' := '$fmtd_5_convertState' / '$scratch'
 656:    let var '$scratch' := '$scratch' * '$charToUart'
 657:    let var '$fmtd_5_convertState' := '$fmtd_5_convertState' - '$scratch'
 658:    let var '$scratch' := 48
 659:    let var '$charToUart' := '$charToUart' + '$scratch'
 660:}
 661:if '$seqScratch' < '0' {
 662:} else {
 663:    if '$fmtd_7_doSend' {
 664:        uart send from '$charToUart'
 665:        increment '$fmtd_4_seq'
 666:    }
 667:}
 668:clear bit '$rung_top'
 669:if '$fmtd_4_seq' < '13' {
 670:    if '$fmtd_7_doSend' {
 671:        set bit '$rung_top'
 672:    }
 673:} else {
 674:    clear bit '$fmtd_7_doSend'
 675:}
 677:# ELEM_ONE_SHOT_FALLING
 678:if not '$rung_top' {
 679:    if '$once_7_ONE_SHOT_FALLING_' {
 680:        clear bit '$once_7_ONE_SHOT_FALLING_'
 681:        set bit '$rung_top'
 682:    }
 683:} else {
 684:    set bit '$once_7_ONE_SHOT_FALLING_'
 685:    clear bit '$rung_top'
 686:}
 688:# ELEM_MOVE
 689:if '$rung_top' {
 690:    let var 'display' := 3
 691:}
 693:# ] finish series
 694:# 
 695:# ======= START RUNG 17 =======
 697:LabelRung17: // 
 698:set bit '$rung_top'
 700:# start series [
 701:# ELEM_EQU
 702:if '$rung_top' {
 703:    if 'display' != '3' {
 704:        clear bit '$rung_top'
 705:    }
 706:}
 708:# ELEM_FORMATTED_STRING
 709:if '$rung_top' {
 710:    if not '$once_8_FMTD_STR_' {
 711:        set bit '$once_8_FMTD_STR_'
 712:        let var '$fmtd_8_seq' := 0
 713:        set bit '$fmtd_b_doSend'
 714:    }
 715:} else {
 716:    clear bit '$once_8_FMTD_STR_'
 717:}
 718:let var '$seqScratch' := '$fmtd_8_seq'
 719:if '$fmtd_8_seq' < '13' {
 720:} else {
 721:    let var '$seqScratch' := -1
 722:}
 723:if '$fmtd_b_doSend' {
 724:    clear bit '$scratch'
 725:    '$scratch' = is uart ready to send ?
 726:    if not '$scratch' {
 727:        let var '$seqScratch' := -1
 728:    }
 729:}
 730:let var '$scratch' := 0
 731:if '$scratch' == '$seqScratch' {
 732:    let var '$charToUart' := 32
 733:}
 734:let var '$scratch' := 1
 735:if '$scratch' == '$seqScratch' {
 736:    let var '$charToUart' := 115
 737:}
 738:let var '$scratch' := 2
 739:if '$scratch' == '$seqScratch' {
 740:    let var '$charToUart' := 97
 741:}
 742:let var '$scratch' := 3
 743:if '$scratch' == '$seqScratch' {
 744:    let var '$charToUart' := 118
 745:}
 746:let var '$scratch' := 4
 747:if '$scratch' == '$seqScratch' {
 748:    let var '$charToUart' := 101
 749:}
 750:let var '$scratch' := 5
 751:if '$scratch' == '$seqScratch' {
 752:    let var '$charToUart' := 100
 753:}
 754:let var '$scratch' := 6
 755:if '$scratch' == '$seqScratch' {
 756:    let var '$charToUart' := 61
 757:}
 758:let var '$scratch' := 7
 759:clear bit '$scratch'
 760:if '$scratch' == '$seqScratch' {
 761:    set bit '$scratch'
 762:}
 763:if '$scratch' {
 764:    let var '$fmtd_9_convertState' := 'saved'
 765:    set bit '$fmtd_a_isLeadingZero'
 766:    let var '$scratch' := 1000
 767:    let var '$charToUart' := '$fmtd_9_convertState' / '$scratch'
 768:    let var '$scratch' := '$scratch' * '$charToUart'
 769:    let var '$fmtd_9_convertState' := '$fmtd_9_convertState' - '$scratch'
 770:    let var '$scratch' := 48
 771:    let var '$charToUart' := '$charToUart' + '$scratch'
 772:    if '$scratch' == '$charToUart' {
 773:        if '$fmtd_a_isLeadingZero' {
 774:            let var '$charToUart' := 32
 775:        }
 776:    } else {
 777:        clear bit '$fmtd_a_isLeadingZero'
 778:    }
 779:}
 780:let var '$scratch' := 8
 781:clear bit '$scratch'
 782:if '$scratch' == '$seqScratch' {
 783:    set bit '$scratch'
 784:}
 785:if '$scratch' {
 786:    let var '$scratch' := 100
 787:    let var '$charToUart' := '$fmtd_9_convertState' / '$scratch'
 788:    let var '$scratch' := '$scratch' * '$charToUart'
 789:    let var '$fmtd_9_convertState' := '$fmtd_9_convertState' - '$scratch'
 790:    let var '$scratch' := 48
 791:    let var '$charToUart' := '$charToUart' + '$scratch'
 792:    if '$scratch' == '$charToUart' {
 793:        if '$fmtd_a_isLeadingZero' {
 794:            let var '$charToUart' := 32
 795:        }
 796:    } else {
 797:        clear bit '$fmtd_a_isLeadingZero'
 798:    }
 799:}
 800:let var '$scratch' := 9
 801:clear bit '$scratch'
 802:if '$scratch' == '$seqScratch' {
 803:    set bit '$scratch'
 804:}
 805:if '$scratch' {
 806:    let var '$scratch' := 10
 807:    let var '$charToUart' := '$fmtd_9_convertState' / '$scratch'
 808:    let var '$scratch' := '$scratch' * '$charToUart'
 809:    let var '$fmtd_9_convertState' := '$fmtd_9_convertState' - '$scratch'
 810:    let var '$scratch' := 48
 811:    let var '$charToUart' := '$charToUart' + '$scratch'
 812:    if '$scratch' == '$charToUart' {
 813:        if '$fmtd_a_isLeadingZero' {
 814:            let var '$charToUart' := 32
 815:        }
 816:    } else {
 817:        clear bit '$fmtd_a_isLeadingZero'
 818:    }
 819:}
 820:let var '$scratch' := 10
 821:clear bit '$scratch'
 822:if '$scratch' == '$seqScratch' {
 823:    set bit '$scratch'
 824:}
 825:if '$scratch' {
 826:    let var '$scratch' := 1
 827:    let var '$charToUart' := '$fmtd_9_convertState' / '$scratch'
 828:    let var '$scratch' := '$scratch' * '$charToUart'
 829:    let var '$fmtd_9_convertState' := '$fmtd_9_convertState' - '$scratch'
 830:    let var '$scratch' := 48
 831:    let var '$charToUart' := '$charToUart' + '$scratch'
 832:}
 833:let var '$scratch' := 11
 834:if '$scratch' == '$seqScratch' {
 835:    let var '$charToUart' := 13
 836:}
 837:let var '$scratch' := 12
 838:if '$scratch' == '$seqScratch' {
 839:    let var '$charToUart' := 10
 840:}
 841:if '$seqScratch' < '0' {
 842:} else {
 843:    if '$fmtd_b_doSend' {
 844:        uart send from '$charToUart'
 845:        increment '$fmtd_8_seq'
 846:    }
 847:}
 848:clear bit '$rung_top'
 849:if '$fmtd_8_seq' < '13' {
 850:    if '$fmtd_b_doSend' {
 851:        set bit '$rung_top'
 852:    }
 853:} else {
 854:    clear bit '$fmtd_b_doSend'
 855:}
 857:# ELEM_ONE_SHOT_FALLING
 858:if not '$rung_top' {
 859:    if '$once_9_ONE_SHOT_FALLING_' {
 860:        clear bit '$once_9_ONE_SHOT_FALLING_'
 861:        set bit '$rung_top'
 862:    }
 863:} else {
 864:    set bit '$once_9_ONE_SHOT_FALLING_'
 865:    clear bit '$rung_top'
 866:}
 868:# ELEM_MOVE
 869:if '$rung_top' {
 870:    let var 'display' := 0
 871:}
 873:# ] finish series
 874:# 
 875:# ======= START RUNG 18 =======
 877:LabelRung18: // 
 878:# by Ihor Nehrutsa, any questions to LDmicro.GitHub@gmail.com, Ap
 879:# Tested at ATmega328p on the Arduino Nano.
 881:LabelRung19: // 
