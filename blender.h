// This file seems wrong and cursed (makes the lsp go crazy) but it's just because Mouse and Keyboard are singletons which must be initialized during setup() in the main source file

int blender_apply_zoom(int arg)
{
	Serial.print(arg);
}
