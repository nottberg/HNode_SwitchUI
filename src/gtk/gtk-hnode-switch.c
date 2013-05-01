#include <gtk/gtk.h>

#include <hnode-1.0/hnode-browser.h>
#include <hnode-1.0/hnode-pktsrc.h>
#include <hnode-1.0/hnode-cfginf.h>
#include <hnode-1.0/hnode-uid.h>

#include "hnode-switch-interface.h"

enum
{
    SWNODE_STATE_NOINIT,
    SWNODE_STATE_SWLIST,
    SWNODE_STATE_READY,
};

typedef struct HNodeSwitchNodeRecord
{
    GHNodeBrowserHNode *BrowserNode;
    guint32             EPIndex;
    guint32             State;
    GHNodeAddress      *EPAddrObj;
    guint32             NodeID;
}HNODE_SWITCH_NODE;

typedef struct HNodeSwitchRecord
{
    guint32            SwitchIDLength;
    gchar             *SwitchIDStr;
    guint32            NameLength;
    gchar             *NameStr;
    guint32            SwitchIndex;
    guint32            CapFlags;
    HNODE_SWITCH_NODE *ParentNode;
}HNODE_SWITCH;

typedef struct HNodeSwitchRequest
{
    guint32            ExpectedResponse;
    guint32            ReqTag;
    HNODE_SWITCH      *Switch;
    HNODE_SWITCH_NODE *RequestNode;
}HNODE_SWITCH_REQUEST;

typedef struct HNodeSwitchCmdlineContext
{
   GMainLoop     *loop; 
   GHNodeBrowser *Browser;

   GHNodePktSrc       *SwitchSource;
   GHNodeAddress      *SwitchAddrObj;

   guint32 ReqTag;
   guint32 ReqState;
   guint32 ReqTID;

   GArray *SwitchNodeArray;
   GArray *SwitchArray;

   GList  *ReqList;

   GtkWidget    *window;
   GtkWidget    *statusbar;

   GtkWidget    *OnButton;
   GtkWidget    *OffButton;

   GtkWidget    *treeview;
   GtkTreeStore *store;

   GtkTreeIter  SwitchIter;
   GtkTreeIter  SceneIter;
   GtkTreeIter  ScheduleIter;
   GtkTreeIter  EventIter;

   GtkTreeSelection *selection;

}CONTEXT;

// Create a global program context.
CONTEXT  gContext;

enum
{
    ETYPE_SWITCH_CONTROL,
    ETYPE_SCENE_CONTROL,
    ETYPE_SCHEDULE_CONTROL,
    ETYPE_EVENT_CONTROL,
    ETYPE_SWITCH,
};

enum
{
   CONTROL_COLUMN,
   DESCRIPTION_COLUMN,
   STATUS_COLUMN,
   TYPE_COLUMN,
   INDEX_COLUMN,
   N_COLUMNS
};

static void 
tree_selection_changed( GtkTreeSelection *selection, gpointer data )
{
   CONTEXT *Context = data;

   GtkTreeIter iter;
   GtkTreeModel *model;
   HNODE_SWITCH *SwitchPtr;
   guint SwitchIndex;
   guint TypeValue;

   if (gtk_tree_selection_get_selected ( Context->selection, &model, &iter))
   {
        gtk_tree_model_get (model, &iter, TYPE_COLUMN, &TypeValue, INDEX_COLUMN, &SwitchIndex, -1);

        if( TypeValue == ETYPE_SWITCH )
        {
            gtk_widget_set_sensitive( Context->OnButton, TRUE);
            gtk_widget_set_sensitive( Context->OffButton, TRUE);
        }
        else
        {
            gtk_widget_set_sensitive( Context->OnButton, FALSE);
            gtk_widget_set_sensitive( Context->OffButton, FALSE);
        }
   }

}

void
populate_static_tree_model(CONTEXT *Context)
{
    GtkTreeIter iter1;  // Parent iter 

    gtk_tree_store_append (Context->store, &Context->SwitchIter, NULL);  // Acquire a top-level iterator 
    gtk_tree_store_set (Context->store, &Context->SwitchIter,
                    CONTROL_COLUMN, "Switch",
                    DESCRIPTION_COLUMN, "",
                    STATUS_COLUMN, "",
                    TYPE_COLUMN, ETYPE_SWITCH_CONTROL,
                    INDEX_COLUMN, 0,
                    -1);
/*
    gtk_tree_store_append (Context->store, &Context->SceneIter, NULL);  // Acquire a top-level iterator 
    gtk_tree_store_set (Context->store, &Context->SceneIter,
                    CONTROL_COLUMN, "Scene",
                    DESCRIPTION_COLUMN, "",
                    STATUS_COLUMN, "",
                    TYPE_COLUMN, ETYPE_SCENE_CONTROL,
                    INDEX_COLUMN, 0,
                    -1);

    gtk_tree_store_append (Context->store, &Context->ScheduleIter, NULL);  // Acquire a top-level iterator 
    gtk_tree_store_set (Context->store, &Context->ScheduleIter,
                    CONTROL_COLUMN, "Scheduled",
                    DESCRIPTION_COLUMN, "",
                    STATUS_COLUMN, "",
                    TYPE_COLUMN, ETYPE_SCHEDULE_CONTROL,
                    INDEX_COLUMN, 0,
                    -1);

    gtk_tree_store_append (Context->store, &Context->EventIter, NULL);  // Acquire a top-level iterator 
    gtk_tree_store_set (Context->store, &Context->EventIter,
                    CONTROL_COLUMN, "Event",
                    DESCRIPTION_COLUMN, "",
                    STATUS_COLUMN, "",
                    TYPE_COLUMN, ETYPE_EVENT_CONTROL,
                    INDEX_COLUMN, 0,
                    -1);
*/
}

void
setup_tree (CONTEXT *Context)
{
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    /* Create a model.  We are using the store model for now, though we
    * could use any other GtkTreeModel */
    Context->store = gtk_tree_store_new (N_COLUMNS,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_UINT,
                               G_TYPE_UINT);

    /* custom function to fill the model with data */
    populate_static_tree_model (Context);

    /* Create a view */
    Context->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (Context->store));

    /* The view now holds a reference.  We can get rid of our own
    * reference */
    g_object_unref (G_OBJECT (Context->store));

    /* Create a cell render and arbitrarily make it red for demonstration
    * purposes */
    renderer = gtk_cell_renderer_text_new ();
    //g_object_set (G_OBJECT (renderer),
    //              "foreground", "red",
    //              NULL);

    /* Create a column, associating the "text" attribute of the
    * cell_renderer to the first column of the model */
    column = gtk_tree_view_column_new_with_attributes ("Control", renderer,
                                                      "text", CONTROL_COLUMN,
                                                      NULL);

    /* Add the column to the view. */
    gtk_tree_view_append_column (GTK_TREE_VIEW (Context->treeview), column);

    /* Second column.*/
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Description",
                                                      renderer,
                                                      "text", DESCRIPTION_COLUMN,
                                                      NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (Context->treeview), column);

    /* Third column.*/
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Status",
                                                      renderer,
                                                      "text", STATUS_COLUMN,
                                                      NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (Context->treeview), column);

    // Get the selection indicator
    Context->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (Context->treeview));
    gtk_tree_selection_set_mode (Context->selection, GTK_SELECTION_SINGLE);

    g_signal_connect (G_OBJECT (Context->selection), "changed",
                      G_CALLBACK (tree_selection_changed),
                      Context);

}


void switch_on(GtkWidget *widget, gpointer data)
{
   CONTEXT *Context = data;

   GtkTreeIter iter;
   GtkTreeModel *model;
   HNODE_SWITCH *SwitchPtr;
   guint SwitchIndex;
   guint TypeValue;
   gchar TmpStr[128];

   if (gtk_tree_selection_get_selected ( Context->selection, &model, &iter))
   {
        gtk_tree_model_get (model, &iter, TYPE_COLUMN, &TypeValue, INDEX_COLUMN, &SwitchIndex, -1);

        if( TypeValue == ETYPE_SWITCH )
        {
            SwitchPtr = &g_array_index(Context->SwitchArray, HNODE_SWITCH, SwitchIndex);

            g_sprintf(TmpStr, "Turning On %s...", SwitchPtr->NameStr);

            gtk_statusbar_push(GTK_STATUSBAR(Context->statusbar),
                    gtk_statusbar_get_context_id(GTK_STATUSBAR(Context->statusbar), TmpStr), TmpStr);

            swcfg_request_switch_on_off( Context, SwitchPtr, TRUE );
        }

   }
}

void switch_off(GtkWidget *widget, gpointer data)
{
   CONTEXT *Context = data;

   GtkTreeIter iter;
   GtkTreeModel *model;
   HNODE_SWITCH *SwitchPtr;
   guint SwitchIndex;
   guint TypeValue;
   gchar TmpStr[128];

   if (gtk_tree_selection_get_selected ( Context->selection, &model, &iter))
   {
        gtk_tree_model_get (model, &iter, TYPE_COLUMN, &TypeValue, INDEX_COLUMN, &SwitchIndex, -1);

        if( TypeValue == ETYPE_SWITCH )
        {
            SwitchPtr = &g_array_index(Context->SwitchArray, HNODE_SWITCH, SwitchIndex);

            g_sprintf(TmpStr, "Turning Off %s...", SwitchPtr->NameStr);

            gtk_statusbar_push(GTK_STATUSBAR(Context->statusbar),
                    gtk_statusbar_get_context_id(GTK_STATUSBAR(Context->statusbar), TmpStr), TmpStr);

            swcfg_request_switch_on_off( Context, SwitchPtr, FALSE );
        }

   }
}

gboolean
swcfg_request_switch_list( CONTEXT *Context, HNODE_SWITCH_NODE *SwitchNode )
{
    GHNodePacket *Packet;

    guint32            ExpectedResponse;
    guint32            ReqTag;
    HNODE_SWITCH      *Switch;
    HNODE_SWITCH_NODE *RequestNode;
    
    HNODE_SWITCH_REQUEST *ReqRecord;

    // Fire off the status request to the configuration interface.
    Packet    = g_hnode_packet_new();
    ReqRecord = g_malloc(sizeof(HNODE_SWITCH_REQUEST));

    // Reset the insertion point to zero.
    g_hnode_packet_reset(Packet);
        
    // Set-up the destination address.
    g_hnode_packet_assign_addrobj(Packet, SwitchNode->EPAddrObj);

    // Set-up the request record
    Context->ReqTag += 1;
    ReqRecord->ExpectedResponse = SWPKT_SWITCH_LIST_REPLY;
    ReqRecord->ReqTag      = Context->ReqTag;
    ReqRecord->Switch      = NULL;
    ReqRecord->RequestNode = SwitchNode;

    Context->ReqList = g_list_append(Context->ReqList, ReqRecord);

    // Build the packet IU.
    g_hnode_packet_set_uint(Packet, SWPKT_SWITCH_LIST_REQUEST);
    g_hnode_packet_set_uint(Packet, ReqRecord->ReqTag);

    // Send the packet
    g_hnode_pktsrc_send_packet(Context->SwitchSource, Packet);

    // Succeeded
    return FALSE;
}

gboolean
swcfg_request_switch_on_off( CONTEXT *Context, HNODE_SWITCH *SwitchRecord, gboolean OnRequest )
{
    GHNodePacket *Packet;

    guint32            ExpectedResponse;
    guint32            ReqTag;
    HNODE_SWITCH      *Switch;
    HNODE_SWITCH_NODE *RequestNode;
    
    HNODE_SWITCH_REQUEST *ReqRecord;

    // Fire off the status request to the configuration interface.
    Packet    = g_hnode_packet_new();
    ReqRecord = g_malloc(sizeof(HNODE_SWITCH_REQUEST));

    // Reset the insertion point to zero.
    g_hnode_packet_reset(Packet);
        
    // Set-up the destination address.
    g_hnode_packet_assign_addrobj(Packet, SwitchRecord->ParentNode->EPAddrObj);

    // Set-up the request record
    Context->ReqTag += 1;
    ReqRecord->ExpectedResponse = SWPKT_SWITCH_CMD_REPLY;
    ReqRecord->ReqTag      = Context->ReqTag;
    ReqRecord->Switch      = SwitchRecord;
    ReqRecord->RequestNode = SwitchRecord->ParentNode;

    Context->ReqList = g_list_append(Context->ReqList, ReqRecord);

    // Build the packet IU.
    g_hnode_packet_set_uint(Packet, SWPKT_SWITCH_CMD_REQUEST);
    g_hnode_packet_set_uint(Packet, ReqRecord->ReqTag);

    g_hnode_packet_set_short(Packet, SwitchRecord->SwitchIndex);  // SwitchIndex
    g_hnode_packet_set_short(Packet, OnRequest ? SWINF_CMD_TURN_ON : SWINF_CMD_TURN_OFF);  // Requested Action
    g_hnode_packet_set_short(Packet, 0);  // Action Parameter 1
    g_hnode_packet_set_short(Packet, 0);  // Action Paremeter 2

    // Send the packet
    g_hnode_pktsrc_send_packet(Context->SwitchSource, Packet);

    // Succeeded
    return FALSE;
}

void 
swcfg_hnode_rx(GHNodePktSrc *Browser, GHNodePacket *Packet, gpointer data)
{
    CONTEXT *Context = data;
    guint    DataLength;
    guint    PktType;
    guint    ReqTag;
    guint    i, j;
    guint    SwitchCount;
    gchar    AttrStr[256];
    guint    MemSegID;
    guint8   TmpBuf[512];

    GList    *CurReq;
    HNODE_SWITCH_REQUEST *ReqRecord;
    HNODE_SWITCH  SwitchRecord;
    GtkTreeIter iter2;  /* Child iter  */

    // Turn off the request timer if one was enabled.
    if( Context->ReqTID )
    {
        g_source_remove( Context->ReqTID ); 
        Context->ReqTID = 0;
    }

    // Parse the rx frame
    DataLength = g_hnode_packet_get_data_length(Packet);

    // Pull out the common fields.
    PktType    = g_hnode_packet_get_uint(Packet);
    ReqTag     = g_hnode_packet_get_uint(Packet);

    // Find the request record that this reply is for.
    ReqRecord = NULL;
    CurReq = g_list_first(Context->ReqList);
    while( CurReq )
    {
        if(((HNODE_SWITCH_REQUEST *)CurReq->data)->ReqTag == ReqTag)
        {
           ReqRecord = CurReq->data;
           break;         
        }    
        CurReq = g_list_next(CurReq);
    }

    // Error check
    if( (ReqRecord == NULL) || (PktType != ReqRecord->ExpectedResponse) )
    {
        // Free the RX frame
        g_object_unref(Packet);
        return;
    }

    // Act on the new packet
    switch( PktType )
    {
        case SWPKT_SWITCH_LIST_REPLY:
            // Number of switch records from this node.
            SwitchCount = g_hnode_packet_get_short(Packet); // Number of switch records.

            for(i = 0; i < SwitchCount; i++)
            {
                SwitchRecord.SwitchIndex   = g_hnode_packet_get_short(Packet);  // Local SwitchID

  		        SwitchRecord.SwitchIDLength = g_hnode_packet_get_short(Packet); // Switch Name Length
                SwitchRecord.SwitchIDStr = NULL;
                if( SwitchRecord.SwitchIDLength )
                {
                    g_hnode_packet_get_bytes(Packet, TmpBuf, SwitchRecord.SwitchIDLength);
                    TmpBuf[SwitchRecord.SwitchIDLength] = '\0';
                    SwitchRecord.SwitchIDStr = g_strdup(TmpBuf);
                }

  		        SwitchRecord.NameLength = g_hnode_packet_get_short(Packet); // Switch Name Length
                SwitchRecord.NameStr = NULL;
                if( SwitchRecord.NameLength )
                {
                    g_hnode_packet_get_bytes(Packet, TmpBuf, SwitchRecord.NameLength);
                    TmpBuf[SwitchRecord.NameLength] = '\0';
                    SwitchRecord.NameStr = g_strdup(TmpBuf);
                }

                SwitchRecord.CapFlags   = g_hnode_packet_get_uint(Packet); 

                SwitchRecord.ParentNode = ReqRecord->RequestNode;

                Context->SwitchArray = g_array_append_val(Context->SwitchArray, SwitchRecord);

                gtk_tree_store_append (Context->store, &iter2, &Context->SwitchIter);  /* Acquire a child iterator */
                gtk_tree_store_set (Context->store, &iter2,
                                        CONTROL_COLUMN, "",
                                        DESCRIPTION_COLUMN, SwitchRecord.NameStr,
                                        STATUS_COLUMN, "Unknown",
                                        TYPE_COLUMN, ETYPE_SWITCH,
                                        INDEX_COLUMN, Context->SwitchArray->len-1,
                                        -1);

                gtk_tree_view_expand_to_path( GTK_TREE_VIEW(Context->treeview), gtk_tree_model_get_path( GTK_TREE_MODEL (Context->store), &iter2 ) );
            }
        break;

        case SWPKT_SWITCH_CMD_REPLY:
            //g_print("Cmd Result: 0x%x\n", g_hnode_packet_get_short(Packet));
            
            gtk_statusbar_push(GTK_STATUSBAR(Context->statusbar),
                    gtk_statusbar_get_context_id(GTK_STATUSBAR(Context->statusbar), "Complete"), "Complete");

        break;
    }

    // Free the ReqRecord
    Context->ReqList = g_list_remove(Context->ReqList, CurReq);
    g_free(ReqRecord);

    // Free the RX frame
    g_object_unref(Packet);

	return;
}
 
void 
swcfg_hnode_tx(GHNodePktSrc *Browser, GHNodePacket *Packet, gpointer data)
{
 
    // Free the TX frame
    g_object_unref(Packet);

	return;
}

static void
mgmt_add (GHNodeBrowser *Browser, GHNodeBrowserMPoint *MPoint, void *userdata)
{
    CONTEXT *Context = userdata;
}

static void
hnode_add (GHNodeBrowser *Browser, GHNodeBrowserHNode *hnode, void *userdata)
{
    CONTEXT *Context = userdata;
    guint  i;
    guint32  EPIndex;
    HNODE_SWITCH_NODE  SwitchNode;
    HNODE_SWITCH_NODE *SwitchPtr;

    g_print("hnode add\n");

    // Check if this hnode is a switch interface node.
    if( g_hnode_browser_hnode_supports_interface(hnode, "hnode-switch-interface", &EPIndex) == TRUE )
    { 
        // If it is then remember it's HNode and request it's switch list.
        SwitchNode.BrowserNode = hnode;
        SwitchNode.EPIndex     = EPIndex;
        SwitchNode.State       = SWNODE_STATE_SWLIST;

        // Get the address object to use when building outbound packets.
        g_hnode_browser_hnode_endpoint_get_address(hnode, EPIndex, &SwitchNode.EPAddrObj); 

        // Add the new switch node to the array
        Context->SwitchNodeArray = g_array_append_val(Context->SwitchNodeArray, SwitchNode);

        // Request a switch list from the node.
        SwitchPtr = &g_array_index(Context->SwitchNodeArray, HNODE_SWITCH_NODE, Context->SwitchNodeArray->len-1);
        swcfg_request_switch_list( Context, SwitchPtr );
    }
}

static gchar *hnodeAddrStr = NULL;

static GOptionEntry entries[] = 
{
  { "hnode", 'n', 0, G_OPTION_ARG_STRING, &hnodeAddrStr, "Connect directly to a switch hnode by address.", NULL },
  { NULL }
};

static gboolean
timeout_static_hnode(void *userdata)
{
    CONTEXT *Context = userdata;
    HNODE_SWITCH_NODE  SwitchNode;
    HNODE_SWITCH_NODE *SwitchPtr;

    g_print("static hnode add\n");

    // If it is then remember it's HNode and request it's switch list.
    SwitchNode.BrowserNode = NULL;
    SwitchNode.EPIndex     = 0;
    SwitchNode.State       = SWNODE_STATE_SWLIST;

    // Create an address object from the specified address.
    SwitchNode.EPAddrObj = g_hnode_address_new();
    g_hnode_address_set_str( SwitchNode.EPAddrObj, hnodeAddrStr );

    // Add the new switch node to the array
    Context->SwitchNodeArray = g_array_append_val(Context->SwitchNodeArray, SwitchNode);

    // Request a switch list from the node.
    SwitchPtr = &g_array_index(Context->SwitchNodeArray, HNODE_SWITCH_NODE, 0);
    swcfg_request_switch_list( Context, SwitchPtr );

    // Dont reschedule
    return FALSE;
}

int main( int argc, char *argv[])
{

  //GtkWidget *window;
  GtkWidget *bbox;
  //GtkWidget *statusbar;
  GtkWidget *vbox;
  GtkWidget *hbox;
  //GtkWidget *treeview;
  GtkWidget *icon;
  GError *error = NULL;
  GOptionContext *cmdline_context;
   
  gtk_init(&argc, &argv);


  // Initialize some context.
  gContext.SwitchSource  = NULL;
  gContext.SwitchAddrObj = NULL;
  gContext.ReqTag        = 0;
  gContext.ReqState      = 0;
  gContext.ReqTID        = 0;

  gContext.SwitchNodeArray = g_array_new( FALSE, TRUE, sizeof(HNODE_SWITCH_NODE) );
  gContext.SwitchArray = g_array_new( FALSE, TRUE, sizeof(HNODE_SWITCH) );

  gContext.ReqList = NULL;


  // Allocate a socket that will be used to converse with a the HNode configuration interface.
  gContext.SwitchSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
  g_signal_connect(gContext.SwitchSource, "rx_packet", G_CALLBACK(swcfg_hnode_rx), &gContext);
  g_signal_connect(gContext.SwitchSource, "tx_packet", G_CALLBACK(swcfg_hnode_tx), &gContext);

  g_hnode_pktsrc_start( gContext.SwitchSource );

  // Parse any command line options.
  cmdline_context = g_option_context_new ("- GUI for hnode switch controllers.");
  g_option_context_add_main_entries (cmdline_context, entries, NULL); // GETTEXT_PACKAGE);
  g_option_context_parse (cmdline_context, &argc, &argv, &error);

  // Check if the browser should be used, or a static address was specified.
  if( hnodeAddrStr == NULL )
  {  
        // Setup the hnode browser
        gContext.Browser = g_hnode_browser_new ();
	
        g_signal_connect(gContext.Browser, "hnode-add", G_CALLBACK(hnode_add), &gContext);
        g_signal_connect(gContext.Browser, "mgmt-add", G_CALLBACK(mgmt_add), &gContext);
        //g_signal_connect(gContext.Browser, "enumeration-complete", G_CALLBACK(enumeration_complete), &gContext);
            
        // Start up the discovery stuff.
        g_hnode_browser_start(gContext.Browser);
  }
  else
  {
        // Set a 0 timeout to kick things off
        g_timeout_add( 0, timeout_static_hnode, &gContext );
  }


  // Build the UI
  gContext.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(gContext.window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(gContext.window), 280, 150);
  gtk_window_set_title(GTK_WINDOW(gContext.window), "Switch Control");

  vbox = gtk_vbox_new(FALSE, 3);
  //hbox = gtk_hbox_new(FALSE, 2);

  //gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 1);

  bbox = gtk_toolbar_new();
  
  gtk_container_add(GTK_CONTAINER(gContext.window), vbox);
  gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 1);

  icon = gtk_image_new_from_icon_name( "gtk-go-up", GTK_ICON_SIZE_LARGE_TOOLBAR );
  gContext.OnButton = (GtkWidget *) gtk_tool_button_new(icon, "On");
  gtk_widget_set_sensitive( gContext.OnButton, FALSE);

  icon = gtk_image_new_from_icon_name( "gtk-go-down", GTK_ICON_SIZE_LARGE_TOOLBAR );
  gContext.OffButton = (GtkWidget *) gtk_tool_button_new(icon, "Off");
  gtk_widget_set_sensitive( gContext.OffButton, FALSE);

  gtk_toolbar_insert(GTK_TOOLBAR(bbox), GTK_TOOL_ITEM(gContext.OnButton), -1);
  gtk_toolbar_insert(GTK_TOOLBAR(bbox), GTK_TOOL_ITEM(gContext.OffButton), -1);

  setup_tree( &gContext );
  gtk_box_pack_start(GTK_BOX(vbox), gContext.treeview, TRUE, TRUE, 1);

  gContext.statusbar = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(vbox), gContext.statusbar, FALSE, TRUE, 1);

  g_signal_connect(G_OBJECT(gContext.OnButton), "clicked", 
           G_CALLBACK(switch_on), &gContext);

  g_signal_connect(G_OBJECT(gContext.OffButton), "clicked", 
           G_CALLBACK(switch_off), &gContext);

  g_signal_connect_swapped(G_OBJECT(gContext.window), "destroy",
        G_CALLBACK(gtk_main_quit), G_OBJECT(gContext.window));

  gtk_widget_show_all(gContext.window);

  gtk_main();

  return 0;
}

