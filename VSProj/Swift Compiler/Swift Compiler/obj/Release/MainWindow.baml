   M S B A M L   `   `   `  ���� L  HSwift for Windows, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null/clr-namespace:Swift_CompilerSwift_Compiler      Swift_Compiler.MainWindow   \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     @d2http://schemas.microsoft.com/expression/blend/2008     Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006     .localclr-namespace:Swift_Compiler       � Title$  Swift for Windows��$	��600��$	��600��$	��600��$
��1000��$
��1000��$	��100��.��� .+��� -    ��Name$ groupBoxCompiler��$��Compiler��$��	10,5,10,0q�$	��Top=�$��Stretch=�$	��138��$��18.667(����	#FFFC3524�� -    ��Name$ textBoxSelectedFile��$
��Left=�$��35��$��	30,42,0,0q� �� TextWrapping$
 Wrap=�$)��#Select swift file to compile or run��$	��Top=�$	��700��$��21.333(�{�.  ���	#FF009FFF�� -    ��Name$ buttonSelectFile��$��Select File��$��Right=�$��
10,42,30,0q�$	��Top=�$	��220��$��35��$��18.667(����	#FF009FFF�� -   $ buttonCompile��$��Compile��$��	30,90,0,0q�$	��Top=�$	��460��$��40��$��26.667(�$
��Left=����	#FF009FFF�� -   $ 	buttonRun��$	��Run��$��Right=�$��
10,90,30,0q�$	��Top=�$	��460��$��40��$��26.667(����	#FF009FFF�� -   $ groupBoxSetting��$��Compiler Settings��$��10,150,10,0q�$	��Top=�$��Stretch=�$	��150�����	#FFFC3524$��18.667(��� -   $ buttonResetSetting��$��Reset Settings��$��Right=�$	��Top=�$��79�����	#FF3399FF$��Right=�$��
0,180,33,0q���Z� ��Z� �� -    ��Name$ 	labelHint��$<��6* double click on text field to change settings values��$
��Left=�$��
20,180,0,0q�$	��Top=��� -	   $ label��$��VC++ for VS2015�� � ToolTip$7 1Visual C++ Redistributable for Visual Studio 2015��$
��Left=�$��
20,205,0,0q�$	��Top=�$��18.667(��� -
    ��Name$ comboBoxProcessor�� �� SelectedValuePath$ Content��	 �� Text$	 	Processor��$ Select Processor Type��$��Right=�$��628,207,30,0q�$	��Top=�$	��220�����	#FF009FFF$��30��$��16(�.@��� -   $	��x86��.��x86�� -   $��amd64��.��amd64�� -   $	��x64��.��x64�� -   $	��arm��.��arm�� -   $��x86_arm��.��	x86_arm�� -   $��	x86_amd64��.��	x86_amd64�� -   $��	amd64_x86��.��	amd64_x86�� -   $��	amd64_arm��.��	amd64_arm�� -   $ textBoxVCPPVS��$
��Left=�$��30��$��200,207,0,0q�$
 Wrap=�$7��1Visual C++ Redistributable for Visual Studio 2015��$	��Top=�$	��530��$��18(����	#FF009FFF
 �� 
IsReadOnly
 . �� -   $ 
label_Copy��$��Swift Compiler��$ Swift Compiler��$
��Left=�$��
20,252,0,0q�$	��Top=�$��18.667(��� -   $ textBoxSwiftCompiler��$
��Left=�$��30��$��200,255,0,0q�$
 Wrap=�$��Swift Compiler��$	��Top=�$	��762��$��18(����	#FF009FFF
 . �� -   $ groupBoxLogs��$
��Logs��$��
10,0,10,40q�$��Bottom=�$��Stretch=�$	��210�����	#FFFC3524$��18.667(�.���� .���� -    ��Name$ textBlockLog��$��Stretch=�$��0q�$
��Wrap=�$��Stretch=�$��14.667(�$
��Auto=�$
��Auto=���. �� -   $ buttonProjectNews��$��Project Latest News��$��8,0,0,8q�$��Bottom=�$	��110�����	#FF3399FF$
��Left=���Z� ��Z� �� -   $ buttonProjectWebsite��$��Project Website��$��0,0,11,8q�$��Bottom=�$��91�����	#FF3399FF$��Right=���Z� ��Z� �� -   $ buttonProjectHelp��$
��Help��$��0,0,0,8q�$��Bottom=�$��28�����	#FF3399FF$��Center=���Z� ��Z� 