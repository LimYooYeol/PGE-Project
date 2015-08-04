
// This file has been generated by the GUI designer. Do not modify.

public partial class MainWindow
{
	private global::Gtk.UIManager UIManager;
	
	private global::Gtk.Action PGEManagerAction;
	
	private global::Gtk.Action dialogInfoAction;
	
	private global::Gtk.Action ExitAction;
	
	private global::Gtk.VBox vbox1;
	
	private global::Gtk.MenuBar menubar1;
	
	private global::Gtk.Notebook notebook2;
	
	private global::Gtk.ScrolledWindow GtkScrolledWindow1;
	
	private global::Gtk.TreeView pgeNewsTree;
	
	private global::Gtk.Label label6;
	
	private global::Gtk.VBox vbox4;
	
	private global::Gtk.ScrolledWindow GtkScrolledWindow;
	
	private global::Gtk.TreeView configListTreeview;
	
	private global::Gtk.HBox hbox3;
	
	private global::Gtk.Button button3;
	
	private global::Gtk.Label label3;
	
	private global::PGEManager.LaunchEditorWidget launcheditorwidget1;

	protected virtual void Build ()
	{
		global::Stetic.Gui.Initialize (this);
		// Widget MainWindow
		this.UIManager = new global::Gtk.UIManager ();
		global::Gtk.ActionGroup w1 = new global::Gtk.ActionGroup ("Default");
		this.PGEManagerAction = new global::Gtk.Action ("PGEManagerAction", "PGE Manager", null, null);
		this.PGEManagerAction.ShortLabel = "PGE Manager";
		w1.Add (this.PGEManagerAction, null);
		this.dialogInfoAction = new global::Gtk.Action ("dialogInfoAction", "About", null, "gtk-dialog-info");
		this.dialogInfoAction.ShortLabel = "About";
		w1.Add (this.dialogInfoAction, null);
		this.ExitAction = new global::Gtk.Action ("ExitAction", "Exit", null, null);
		this.ExitAction.ShortLabel = "Exit";
		w1.Add (this.ExitAction, null);
		this.UIManager.InsertActionGroup (w1, 0);
		this.AddAccelGroup (this.UIManager.AccelGroup);
		this.Name = "MainWindow";
		this.Title = "MainWindow";
		this.WindowPosition = ((global::Gtk.WindowPosition)(4));
		// Container child MainWindow.Gtk.Container+ContainerChild
		this.vbox1 = new global::Gtk.VBox ();
		this.vbox1.Name = "vbox1";
		this.vbox1.Spacing = 6;
		this.vbox1.BorderWidth = ((uint)(6));
		// Container child vbox1.Gtk.Box+BoxChild
		this.UIManager.AddUiFromString ("<ui><menubar name='menubar1'><menu name='PGEManagerAction' action='PGEManagerAction'><menuitem name='dialogInfoAction' action='dialogInfoAction'/><menuitem name='ExitAction' action='ExitAction'/></menu></menubar></ui>");
		this.menubar1 = ((global::Gtk.MenuBar)(this.UIManager.GetWidget ("/menubar1")));
		this.menubar1.Name = "menubar1";
		this.vbox1.Add (this.menubar1);
		global::Gtk.Box.BoxChild w2 = ((global::Gtk.Box.BoxChild)(this.vbox1 [this.menubar1]));
		w2.Position = 0;
		w2.Expand = false;
		w2.Fill = false;
		// Container child vbox1.Gtk.Box+BoxChild
		this.notebook2 = new global::Gtk.Notebook ();
		this.notebook2.CanFocus = true;
		this.notebook2.Name = "notebook2";
		this.notebook2.CurrentPage = 0;
		this.notebook2.ShowBorder = false;
		// Container child notebook2.Gtk.Notebook+NotebookChild
		this.GtkScrolledWindow1 = new global::Gtk.ScrolledWindow ();
		this.GtkScrolledWindow1.Name = "GtkScrolledWindow1";
		this.GtkScrolledWindow1.ShadowType = ((global::Gtk.ShadowType)(1));
		// Container child GtkScrolledWindow1.Gtk.Container+ContainerChild
		this.pgeNewsTree = new global::Gtk.TreeView ();
		this.pgeNewsTree.CanFocus = true;
		this.pgeNewsTree.Name = "pgeNewsTree";
		this.GtkScrolledWindow1.Add (this.pgeNewsTree);
		this.notebook2.Add (this.GtkScrolledWindow1);
		// Notebook tab
		this.label6 = new global::Gtk.Label ();
		this.label6.Name = "label6";
		this.label6.LabelProp = "PGE News";
		this.notebook2.SetTabLabel (this.GtkScrolledWindow1, this.label6);
		this.label6.ShowAll ();
		// Container child notebook2.Gtk.Notebook+NotebookChild
		this.vbox4 = new global::Gtk.VBox ();
		this.vbox4.Name = "vbox4";
		this.vbox4.Spacing = 6;
		// Container child vbox4.Gtk.Box+BoxChild
		this.GtkScrolledWindow = new global::Gtk.ScrolledWindow ();
		this.GtkScrolledWindow.Name = "GtkScrolledWindow";
		this.GtkScrolledWindow.ShadowType = ((global::Gtk.ShadowType)(1));
		// Container child GtkScrolledWindow.Gtk.Container+ContainerChild
		this.configListTreeview = new global::Gtk.TreeView ();
		this.configListTreeview.CanFocus = true;
		this.configListTreeview.Name = "configListTreeview";
		this.GtkScrolledWindow.Add (this.configListTreeview);
		this.vbox4.Add (this.GtkScrolledWindow);
		global::Gtk.Box.BoxChild w6 = ((global::Gtk.Box.BoxChild)(this.vbox4 [this.GtkScrolledWindow]));
		w6.Position = 0;
		// Container child vbox4.Gtk.Box+BoxChild
		this.hbox3 = new global::Gtk.HBox ();
		this.hbox3.Name = "hbox3";
		this.hbox3.Spacing = 6;
		// Container child hbox3.Gtk.Box+BoxChild
		this.button3 = new global::Gtk.Button ();
		this.button3.CanFocus = true;
		this.button3.Name = "button3";
		this.button3.UseUnderline = true;
		this.button3.Label = ":)";
		this.hbox3.Add (this.button3);
		global::Gtk.Box.BoxChild w7 = ((global::Gtk.Box.BoxChild)(this.hbox3 [this.button3]));
		w7.Position = 2;
		w7.Expand = false;
		w7.Fill = false;
		this.vbox4.Add (this.hbox3);
		global::Gtk.Box.BoxChild w8 = ((global::Gtk.Box.BoxChild)(this.vbox4 [this.hbox3]));
		w8.Position = 1;
		w8.Expand = false;
		w8.Fill = false;
		this.notebook2.Add (this.vbox4);
		global::Gtk.Notebook.NotebookChild w9 = ((global::Gtk.Notebook.NotebookChild)(this.notebook2 [this.vbox4]));
		w9.Position = 1;
		// Notebook tab
		this.label3 = new global::Gtk.Label ();
		this.label3.Name = "label3";
		this.label3.LabelProp = "Config Packs";
		this.notebook2.SetTabLabel (this.vbox4, this.label3);
		this.label3.ShowAll ();
		this.vbox1.Add (this.notebook2);
		global::Gtk.Box.BoxChild w10 = ((global::Gtk.Box.BoxChild)(this.vbox1 [this.notebook2]));
		w10.Position = 1;
		// Container child vbox1.Gtk.Box+BoxChild
		this.launcheditorwidget1 = new global::PGEManager.LaunchEditorWidget ();
		this.launcheditorwidget1.Events = ((global::Gdk.EventMask)(256));
		this.launcheditorwidget1.Name = "launcheditorwidget1";
		this.vbox1.Add (this.launcheditorwidget1);
		global::Gtk.Box.BoxChild w11 = ((global::Gtk.Box.BoxChild)(this.vbox1 [this.launcheditorwidget1]));
		w11.Position = 2;
		w11.Expand = false;
		w11.Fill = false;
		this.Add (this.vbox1);
		if ((this.Child != null)) {
			this.Child.ShowAll ();
		}
		this.DefaultWidth = 626;
		this.DefaultHeight = 461;
		this.Show ();
		this.DeleteEvent += new global::Gtk.DeleteEventHandler (this.OnDeleteEvent);
		this.ExitAction.Activated += new global::System.EventHandler (this.OnExitActionActivated);
	}
}
