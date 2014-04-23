/***************************************************************************

        MC-80.xx driver by Miodrag Milanovic

        15/05/2009 Initial implementation
        12/05/2009 Skeleton driver.
        01/09/2011 Modernised, added a keyboard to mc8020

Real workings of mc8020 keyboard need to be understood and implemented.

mc8030: very little info available. The area from FFD8-FFFF is meant for
interrupt vectors and so on, but most of it is zeroes. Appears the keyboard
is an ascii keyboard with built-in beeper. It communicates via the SIO,
which needs a rewrite to become useful. The asp ctc needs at least 2
triggers. The purpose of the zve pio is unknown. The system uses interrupts
for various things, but none of that is working.

****************************************************************************/

#include "includes/mc80.h"

static ADDRESS_MAP_START(mc8020_mem, AS_PROGRAM, 8, mc80_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0bff) AM_ROM
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("videoram")// 1KB RAM ZRE
	AM_RANGE(0x2000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(mc8020_io, AS_IO, 8, mc80_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf3) AM_DEVREADWRITE("z80ctc", z80ctc_device, read, write)
	AM_RANGE(0xf4, 0xf7) AM_DEVREADWRITE("z80pio", z80pio_device, read, write)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mc8030_mem, AS_PROGRAM, 8, mc80_state)
	ADDRESS_MAP_UNMAP_HIGH
	//  ZRE 4 * 2KB
	AM_RANGE(0x0000, 0x1fff) AM_ROM // ZRE ROM's 4 * 2716
	AM_RANGE(0x2000, 0x27ff) AM_ROM // SPE ROM's 2 * 2708
	AM_RANGE(0x2800, 0x3fff) AM_ROM // For extension
	AM_RANGE(0x4000, 0xbfff) AM_RAM // SPE RAM
	AM_RANGE(0xc000, 0xffff) AM_RAM // ZRE RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(mc8030_io, AS_IO, 8, mc80_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x30, 0x3f) AM_MIRROR(0xff00) AM_NOP //"mass storage"
	AM_RANGE(0x80, 0x83) AM_MIRROR(0xff00) AM_DEVREADWRITE("zve_ctc", z80ctc_device, read, write) // user CTC
	AM_RANGE(0x84, 0x87) AM_MIRROR(0xff00) AM_DEVREADWRITE("zve_pio", z80pio_device, read, write) // PIO unknown usage
	AM_RANGE(0x88, 0x8f) AM_MIRROR(0xff00) AM_WRITE(mc8030_zve_write_protect_w)
	AM_RANGE(0xc0, 0xcf) AM_MIRROR(0xff00) AM_WRITE(mc8030_vis_w) AM_MASK(0xffff)
	AM_RANGE(0xd0, 0xd3) AM_MIRROR(0xff00) AM_DEVREADWRITE("asp_sio", z80sio0_device, cd_ba_r, cd_ba_w) // keyboard & IFSS?
	AM_RANGE(0xd4, 0xd7) AM_MIRROR(0xff00) AM_DEVREADWRITE("asp_ctc", z80ctc_device, read, write) // sio bauds, KMBG? and kbd
	AM_RANGE(0xd8, 0xdb) AM_MIRROR(0xff00) AM_DEVREADWRITE("asp_pio", z80pio_device, read, write) // external bus
	AM_RANGE(0xe0, 0xef) AM_MIRROR(0xff00) AM_WRITE(mc8030_eprom_prog_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mc8020 )
	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('$')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('&')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('\'')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('(')

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('0') PORT_CHAR('\"')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('+') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('^')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('/')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START("X5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("X6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('?')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR(']')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)

	PORT_START("X7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
INPUT_PORTS_END

static INPUT_PORTS_START( mc8030 )
INPUT_PORTS_END


TIMER_DEVICE_CALLBACK_MEMBER(mc80_state::mc8020_kbd)
{
	address_space &mem = m_maincpu->space(AS_PROGRAM);
	char kbdrow[6];
	UINT8 i;
	for (i = 1; i < 8; i++)
	{
		sprintf(kbdrow,"X%X", i);
		mem.write_word(0xd20+i, ioport(kbdrow)->read());
	}
}

// this is a guess there is no information available
static const z80_daisy_config mc8030_daisy_chain[] =
{
	{ "asp_ctc" },      /* System ctc */
	{ "asp_pio" },      /* System pio */
	{ "asp_sio" },      /* sio */
	{ "zve_pio" },      /* User pio */
	{ "zve_ctc" },      /* User ctc */
	{ NULL }
};

static MACHINE_CONFIG_START( mc8020, mc80_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_2_4576MHz)
	MCFG_CPU_PROGRAM_MAP(mc8020_mem)
	MCFG_CPU_IO_MAP(mc8020_io)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DRIVER(mc80_state,mc8020_irq_callback)

	MCFG_MACHINE_RESET_OVERRIDE(mc80_state,mc8020)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(32*6, 16*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 32*6-1, 0, 16*8-1)
	MCFG_VIDEO_START_OVERRIDE(mc80_state,mc8020)
	MCFG_SCREEN_UPDATE_DRIVER(mc80_state, screen_update_mc8020)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD_BLACK_AND_WHITE("palette")

	/* Devices */
	MCFG_Z80PIO_ADD( "z80pio", XTAL_2_4576MHz, mc8020_z80pio_intf )
	MCFG_Z80CTC_ADD( "z80ctc", XTAL_2_4576MHz / 100, mc8020_ctc_intf )
	MCFG_TIMER_DRIVER_ADD_PERIODIC("mc8020_kbd", mc80_state, mc8020_kbd, attotime::from_hz(50))
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( mc8030, mc80_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_2_4576MHz)
	MCFG_CPU_PROGRAM_MAP(mc8030_mem)
	MCFG_CPU_IO_MAP(mc8030_io)
	MCFG_CPU_CONFIG(mc8030_daisy_chain)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DRIVER(mc80_state,mc8030_irq_callback)

	MCFG_MACHINE_RESET_OVERRIDE(mc80_state,mc8030)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(512, 256)
	MCFG_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
	MCFG_VIDEO_START_OVERRIDE(mc80_state,mc8030)
	MCFG_SCREEN_UPDATE_DRIVER(mc80_state, screen_update_mc8030)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD_BLACK_AND_WHITE("palette")

	/* Devices */
	MCFG_Z80PIO_ADD( "zve_pio", XTAL_2_4576MHz, mc8030_zve_z80pio_intf )
	MCFG_Z80CTC_ADD( "zve_ctc", XTAL_2_4576MHz, mc8030_zve_z80ctc_intf )
	MCFG_Z80PIO_ADD( "asp_pio", XTAL_2_4576MHz, mc8030_asp_z80pio_intf )
	MCFG_Z80CTC_ADD( "asp_ctc", XTAL_2_4576MHz, mc8030_asp_z80ctc_intf )
	MCFG_Z80SIO0_ADD( "asp_sio", 4800, mc8030_asp_z80sio_intf )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mc8020 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "ver1", "Version 1")
	ROMX_LOAD( "s01.rom",     0x0000, 0x0400, CRC(0f1c1a62) SHA1(270c0a9e8e165658f3b09d40a3e8bb3dc1b88184), ROM_BIOS(1))
	ROMX_LOAD( "s02.rom",     0x0400, 0x0400, CRC(93b5811c) SHA1(8559d24072c9b5908a2627ff986d818308f51d59), ROM_BIOS(1))
	ROMX_LOAD( "s03.rom",     0x0800, 0x0400, CRC(3d32c334) SHA1(56d3012595540d03054ad3c6795ed5d929581a04), ROM_BIOS(1))
	ROMX_LOAD( "mo01_v2.rom", 0x2000, 0x0400, CRC(7e47201c) SHA1(db49afdc5c1fe4065a979c56cbdbd3c58f5d942f), ROM_BIOS(1))

	ROM_SYSTEM_BIOS(1, "ver2", "Version 2")
	ROMX_LOAD( "s01.rom",    0x0000, 0x0400, CRC(0f1c1a62) SHA1(270c0a9e8e165658f3b09d40a3e8bb3dc1b88184), ROM_BIOS(2))
	ROMX_LOAD( "s02_v2.rom", 0x0400, 0x0400, CRC(dd26c90a) SHA1(1108c11362fa63d21110a3b17868c1854a318c09), ROM_BIOS(2))
	ROMX_LOAD( "s03_v2.rom", 0x0800, 0x0400, CRC(5b64ee7b) SHA1(3b4cbfcb8e2dedcfd4a3680c81fe6ceb2211b275), ROM_BIOS(2))
	ROMX_LOAD( "mo01.rom",   0x2000, 0x0400, CRC(c65a470f) SHA1(71325fed1a342149b5efc2234ecfc8adfff0a42d), ROM_BIOS(2))

	ROM_SYSTEM_BIOS(2, "ver3", "Version 3")
	ROMX_LOAD( "s01.rom",    0x0000, 0x0400, CRC(0f1c1a62) SHA1(270c0a9e8e165658f3b09d40a3e8bb3dc1b88184), ROM_BIOS(3))
	ROMX_LOAD( "s02_v3.rom", 0x0400, 0x0400, CRC(40c7a694) SHA1(bcdf382e8dad9bb6e06d23ec018e9df55c8d8d0c), ROM_BIOS(3))
	ROMX_LOAD( "s03.rom",    0x0800, 0x0400, CRC(3d32c334) SHA1(56d3012595540d03054ad3c6795ed5d929581a04), ROM_BIOS(3))
	ROMX_LOAD( "mo01_v2.rom",0x2000, 0x0400, CRC(7e47201c) SHA1(db49afdc5c1fe4065a979c56cbdbd3c58f5d942f), ROM_BIOS(3))

	// m02 doesn't exist on board
	ROM_LOAD( "mo03.rom", 0x2800, 0x0400, CRC(29685056) SHA1(39e77658fb7af5a28112341f0893e007d73c1b7a))
	ROM_LOAD( "mo04.rom", 0x2c00, 0x0400, CRC(fd315b73) SHA1(cfb943ec8c884a9b92562d05f92faf06fe42ad68))
	ROM_LOAD( "mo05.rom", 0x3000, 0x0400, CRC(453d6370) SHA1(d96f0849a2da958d7e92a31667178ad140719477))
	ROM_LOAD( "mo06.rom", 0x3400, 0x0400, CRC(6357aba5) SHA1(a4867766f6e14e9fe66f22a6839f17c01058c8af))
	ROM_LOAD( "mo07.rom", 0x3800, 0x0400, CRC(a1eb6021) SHA1(b05b63f02de89f065f337bbe54c5b48244e3a4ba))
	ROM_LOAD( "mo08.rom", 0x3c00, 0x0400, CRC(8221cc32) SHA1(65e0ee4241d39d138205c88374b3bcd127e21511))
	ROM_LOAD( "mo09.rom", 0x4000, 0x0400, CRC(7ad5944d) SHA1(ef2781b114277a09ce0cf2e7decfdb7c48a693e3))
	ROM_LOAD( "mo10.rom", 0x4400, 0x0400, CRC(11de8c76) SHA1(b384d22506ff7e3e28bd2dcc33b7a69617eeb52a))
	ROM_LOAD( "mo11.rom", 0x4800, 0x0400, CRC(370cc672) SHA1(133f6e8bfd4e1ca2e9e0a8e2342084419f895e3c))
	ROM_LOAD( "mo12.rom", 0x4c00, 0x0400, CRC(a3838f2b) SHA1(e3602943700bf5068117946bf86f052f5c587169))
	ROM_LOAD( "mo13.rom", 0x5000, 0x0400, CRC(88b61d12) SHA1(00dd4452b4d4191e589ab58ba924ed21b10f323b))
	ROM_LOAD( "mo14.rom", 0x5400, 0x0400, CRC(2168da19) SHA1(c1ce1263167067d8be0a90604d9c29b7379a0545))
	ROM_LOAD( "mo15.rom", 0x5800, 0x0400, CRC(e32f54c4) SHA1(c3d9ca2204e7adbc625cf96031acb8c1df0447c7))
	ROM_LOAD( "mo16.rom", 0x5c00, 0x0400, CRC(403be935) SHA1(4e74355a78ab090ce180437156fed8e4a1d1c787))
ROM_END

ROM_START( mc8030 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "zve_1.rom", 0x0000, 0x0800, CRC(31ec0159) SHA1(a97ea9eb733c462e77d625a7942134e45d911c0a))
	ROM_LOAD( "zve_2.rom", 0x0800, 0x0800, CRC(5104983d) SHA1(7516274904042f4fc6813aa8b2a75c0a64f9b937))
	ROM_LOAD( "zve_3.rom", 0x1000, 0x0800, CRC(4bcfd727) SHA1(d296e587098e70270ad60db8edaa685af368b849))
	ROM_LOAD( "zve_4.rom", 0x1800, 0x0800, CRC(f949ae43) SHA1(68c324cf5578497db7ae65da5695fcb30493f612))
	ROM_LOAD( "spe_1.rom", 0x2000, 0x0400, CRC(826F609C) SHA1(e77ff6c180f5a6d7756d076173ae264a0e26f066))
	ROM_LOAD( "spe_2.rom", 0x2400, 0x0400, CRC(98320040) SHA1(6baf87e196f1ccdf44912deafa6042becbfb0679))

	ROM_REGION( 0x4000, "vram", ROMREGION_ERASE00 )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY               FULLNAME       FLAGS */
COMP( 198?, mc8020, 0,      0,       mc8020,    mc8020, driver_device,  0,   "VEB Elektronik Gera", "MC-80.21/22", GAME_NO_SOUND)
COMP( 198?, mc8030, mc8020, 0,       mc8030,    mc8030, driver_device,  0,   "VEB Elektronik Gera", "MC-80.30/31", GAME_NOT_WORKING | GAME_NO_SOUND | ORIENTATION_FLIP_X)
