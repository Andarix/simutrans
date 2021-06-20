#region Using directives

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Security;
using WebControls = System.Web.UI.WebControls;
using HTMLControls = System.Web.UI.HtmlControls;
using WinForms = System.Windows.Forms;
using SQLTypes = System.Data.SqlTypes;

#endregion

// To build this, open an elevated Developer Command Prompt and run:
// csc /t:library autoexp.cs

// mscorlib
[assembly: DebuggerDisplay(@"\{Name = {Name} FullName = {FullName}}", Target = typeof(Type))]

// System.Collections
[assembly: DebuggerDisplay(@"\{[{Key,nq}, {Value,nq}]}", Target = typeof(KeyValuePair<,>))]

// System.Drawing
[assembly: DebuggerDisplay(@"\{Name = {Name} Size={Size}}", Target = typeof(Font))]
[assembly: DebuggerDisplay(@"\{Name = {Name}}", Target = typeof(FontFamily))]
[assembly: DebuggerDisplay(@"\{Color = {Color}}", Target = typeof(Pen))]
[assembly: DebuggerDisplay(@"\{X = {X} Y = {Y}}", Target = typeof(Point))]
[assembly: DebuggerDisplay(@"\{X = {X} Y = {Y}}", Target = typeof(PointF))]
[assembly: DebuggerDisplay(@"\{X = {X} Y = {Y} Width = {Width} Height = {Height}}", Target = typeof(Rectangle))]
[assembly: DebuggerDisplay(@"\{X = {X} Y = {Y} Width = {Width} Height = {Height}}", Target = typeof(RectangleF))]
[assembly: DebuggerDisplay(@"\{Width = {Width} Height = {Height}}", Target = typeof(Size))]
[assembly: DebuggerDisplay(@"\{Width = {Width} Height = {Height}}", Target = typeof(SizeF))]
[assembly: DebuggerDisplay(@"\{Color = {Color}}", Target = typeof(SolidBrush))]

// System.Web.UI.WebControls
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WebControls::Button))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WebControls::Label))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WebControls::HyperLink))]
[assembly: DebuggerDisplay(@"\{Text = {Text} Checked = {Checked}}", Target = typeof(WebControls::CheckBox))]
[assembly: DebuggerDisplay(@"\{Text = {Text} Checked = {Checked}}", Target = typeof(WebControls::RadioButton))]
[assembly: DebuggerDisplay(@"\{SelectedDate = {SelectedData}}", Target = typeof(WebControls::Calendar))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WebControls::LinkButton))]

// System.Web.UI.HtmlControls
[assembly: DebuggerDisplay(@"\{Value = {Value}}", Target = typeof(HTMLControls::HtmlInputButton))]
[assembly: DebuggerDisplay(@"\{InnerText = {InnerText}}", Target = typeof(HTMLControls::HtmlGenericControl))]
[assembly: DebuggerDisplay(@"\{Value = {Value}}", Target = typeof(HTMLControls::HtmlTextArea))]
[assembly: DebuggerDisplay(@"\{Value = {Value}}", Target = typeof(HTMLControls::HtmlInputText))]
[assembly: DebuggerDisplay(@"\{Value = {Value} Checked = {Checked}}", Target = typeof(HTMLControls::HtmlInputCheckBox))]
[assembly: DebuggerDisplay(@"\{Value = {Value} Checked = {Checked}}", Target = typeof(HTMLControls::HtmlInputRadioButton))]

// System.Windows.Forms
[assembly: DebuggerDisplay(@"\{ExecutablePath = {ExecutablePath}}", Target = typeof(WinForms::Application))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WinForms::Button))]
[assembly: DebuggerDisplay(@"\{Text = {Text} CheckState = {CheckState}}", Target = typeof(WinForms::CheckBox))]
[assembly: DebuggerDisplay(@"\{SelectedItem = {Text}}", Target = typeof(WinForms::CheckedListBox))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WinForms::DataGrid))]
[assembly: DebuggerDisplay(@"\{Type = {Type} Column = {Column} Row = {Row}}", Target = typeof(WinForms::DataGrid.HitTestInfo))]
[assembly: DebuggerDisplay(@"\{HeaderText = {HeaderText}}", Target = typeof(WinForms::DataGridColumnStyle))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WinForms::DataGridTextBox))]
[assembly: DebuggerDisplay(@"\{HeaderText = {HeaderText}}", Target = typeof(WinForms::DataGridTextBoxColumn))]
[assembly: DebuggerDisplay(@"\{Font = {Font} Color = {Color}}", Target = typeof(WinForms::FontDialog))]
[assembly: DebuggerDisplay(@"\{Value = {Value} Min = {Minimum} Max = {Maximum}}", Target = typeof(WinForms::HScrollBar))]
[assembly: DebuggerDisplay(@"\{InvalidRect = {InvalidRect}}", Target = typeof(WinForms::InvalidateEventArgs))]
[assembly: DebuggerDisplay(@"\{Index = {Index}}", Target = typeof(WinForms::ItemChangedEventArgs))]
[assembly: DebuggerDisplay(@"\{Index = {Index} NewValue = {NewValue} CurrentValue = {CurrentValue}}", Target = typeof(WinForms::ItemCheckEventArgs))]
[assembly: DebuggerDisplay(@"\{KeyData = {KeyData}}", Target = typeof(WinForms::KeyEventArgs))]
[assembly: DebuggerDisplay(@"\{KeyChar = {KeyChar}}", Target = typeof(WinForms::KeyPressEventArgs))]
[assembly: DebuggerDisplay(@"\{LinkText = {LinkText}}", Target = typeof(WinForms::LinkClickedEventArgs))]
[assembly: DebuggerDisplay(@"\{SelectedItem = {Text}}", Target = typeof(WinForms::ListBox))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target= typeof(WinForms::ListViewItem))]
[assembly: DebuggerDisplay(@"\{X = {X} Y = {Y} Button = {Button}}", Target = typeof(WinForms::MouseEventArgs))]
[assembly: DebuggerDisplay(@"\{Value = {Value} Min = {Minimum} Max = {Maximum}}", Target = typeof(WinForms::NumericUpDown))]
[assembly: DebuggerDisplay(@"\{ClipRectangle = {ClipRectangle}}", Target = typeof(WinForms::PaintEventArgs))]
[assembly: DebuggerDisplay(@"\{Value = {Value} Min = {Minimum} Max = {Maximum}}", Target = typeof(WinForms::ProgressBar))]
[assembly: DebuggerDisplay(@"\{Text = {Text} Checked = {Checked}}", Target = typeof(WinForms::RadioButton))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WinForms::RichTextBox))]
[assembly: DebuggerDisplay(@"\{Bounds = {Bounds} WorkingArea = {WorkingArea} Primary = {Primary} DeviceName = {DeviceName}}", Target = typeof(WinForms::Screen))]
[assembly: DebuggerDisplay(@"\{Start = {Start} End = {End}}", Target = typeof(WinForms::SelectionRange))]
[assembly: DebuggerDisplay(@"\{SplitPosition = {SplitPosition} MinExtra = {MinExtra} MinSize = {MinSize}}", Target = typeof(WinForms::Splitter))]
[assembly: DebuggerDisplay(@"\{SplitX = {SplitX} SplitY = {SplitY}}", Target = typeof(WinForms::SplitterEventArgs))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WinForms::TextBox))]
[assembly: DebuggerDisplay(@"\{Interval = {Interval}}", Target = typeof(WinForms::Timer))]
[assembly: DebuggerDisplay(@"\{Value = {Value} Min = {Minimum} Max = {Maximum}}", Target = typeof(WinForms::TrackBar))]
[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WinForms::TreeNode))]
[assembly: DebuggerDisplay(@"\{Value = {Value} Min = {Minimum} Max = {Maximum}}", Target = typeof(WinForms::VScrollBar))]

[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(Exception))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(ApplicationException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(ArgumentException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(ArgumentNullException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(ArgumentOutOfRangeException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(ArithmeticException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(DivideByZeroException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(DllNotFoundException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(ApplicationException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(IndexOutOfRangeException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(InvalidCastException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(MemberAccessException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(MethodAccessException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(NullReferenceException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(StackOverflowException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(SystemException))]
[assembly: DebuggerDisplay(@"\{{Message}:{TypeName}}", Target = typeof(TypeLoadException))]
[assembly: DebuggerDisplay(@"\{{Message}:{FileName}}", Target = typeof(FileLoadException))]
[assembly: DebuggerDisplay(@"\{{Message}:{FileName}}", Target = typeof(FileNotFoundException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(ReflectionTypeLoadException))]
[assembly: DebuggerDisplay(@"\{{Message}}", Target = typeof(SecurityException))]
[assembly: DebuggerDisplay(@"\{Method = {Method}}", Target = typeof(System.Delegate))]

// The attributes below can be useful to uncomment if ToString evaluation is disabled. 
//
//[assembly: DebuggerDisplay(@"\{RGB = {value}}", Target = typeof(Color))]
//[assembly: DebuggerDisplay("{Month}/{Day}/{Year} {Hour}:{Minute}:{Second}", Target = typeof(DateTime))]
//[assembly: DebuggerDisplay(@"\{Value = {value}}", Target = typeof(WinForms::DateTimePicker))]
//[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WinForms::LinkLabel))]
//[assembly: DebuggerDisplay(@"\{Text = {Text}}", Target = typeof(WinForms::Label))]
//[assembly: DebuggerDisplay(@"\{SelectionStart = {selectionStart} SelectionEnd = {selectionEnd}}", Target = typeof(WinForms::MonthCalendar))]
//[assembly: DebuggerDisplay(@"\{InitialDelay = {InitialDelay} ShowAlways = {ShowAlways}}", Target = typeof(WinForms::ToolTip))]
//[assembly: DebuggerDisplay("{m_value}", Target = typeof(SQLTypes::SqlInt64))]
//[assembly: DebuggerDisplay("{m_value}", Target = typeof(SQLTypes::SqlDateTime))]
//[assembly: DebuggerDisplay("{m_value}", Target = typeof(SQLTypes::SqlInt32))]
//[assembly: DebuggerDisplay("{Value}", Target = typeof(SQLTypes::SqlMoney))]
//[assembly: DebuggerDisplay("{m_value}", Target = typeof(SQLTypes::SqlString))]
//[assembly: DebuggerDisplay("{m_value}", Target = typeof(SQLTypes::SqlSingle))]
//[assembly: DebuggerDisplay("{m_value}", Target = typeof(SQLTypes::SqlInt16))]
//[assembly: DebuggerDisplay("{m_value}", Target = typeof(SQLTypes::SqlByte))]
//[assembly: DebuggerDisplay("{m_value}", Target = typeof(SQLTypes::SqlDouble))]
