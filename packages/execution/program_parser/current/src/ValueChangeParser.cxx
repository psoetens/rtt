/***************************************************************************
  tag: Peter Soetens  Mon May 10 19:10:37 CEST 2004  ValueChangeParser.cxx

                        ValueChangeParser.cxx -  description
                           -------------------
    begin                : Mon May 10 2004
    copyright            : (C) 2004 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/

#include "execution/parser-debug.hpp"
#include "execution/parse_exception.hpp"
#include "execution/ValueChangeParser.hpp"

#include "execution/TaskContext.hpp"
#include "execution/Types.hpp"

#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>

#include <corelib/Logger.hpp>

#include <sstream>
#include <iostream>

namespace ORO_Execution
{
    using namespace boost::lambda;
    using namespace detail;
    using namespace ORO_CoreLib;
    using namespace std;

    namespace {
        assertion<std::string> expect_open("Open brace expected.");
        assertion<std::string> expect_close("Closing brace expected (or could not find out what this line means).");
        assertion<std::string> expect_type("Unknown type. Please specify a type.");
        assertion<std::string> expect_def("Expected a type definition. Please specify a type.");
        assertion<std::string> expect_expr("Expected a valid expression.");
        assertion<std::string> expect_ident("Expected a valid identifier.");
        assertion<std::string> expect_init("Expected an initialisation value of the variable.");
        assertion<std::string> expect_cis("Expected a initialisation ('=') of const.");
        assertion<std::string> expect_ais("Expected a initialisation ('=') of alias.");
        assertion<std::string> expect_index("Expected an index: [index].");
        assertion<std::string> expect_integer("Expected a positive integer expression.");
        assertion<std::string> expect_change("Expected a variable assignment after 'set'.");
        assertion<std::string> expect_decl("Expected a declaration list.");
    }


  ValueChangeParser::ValueChangeParser( TaskContext* pc )
      : peername(0),
        type( 0 ), context( pc ), expressionparser( pc ), peerparser( pc ), sizehint(-1),
        typerepos( TypeInfoRepository::instance() )
  {
    BOOST_SPIRIT_DEBUG_RULE( constantdefinition );
    BOOST_SPIRIT_DEBUG_RULE( aliasdefinition );
    BOOST_SPIRIT_DEBUG_RULE( variabledefinition );
    BOOST_SPIRIT_DEBUG_RULE( variableassignment );
    BOOST_SPIRIT_DEBUG_RULE( variablechange );
    BOOST_SPIRIT_DEBUG_RULE( paramdefinition );
    BOOST_SPIRIT_DEBUG_RULE( baredefinition );
    BOOST_SPIRIT_DEBUG_RULE( decl );

    // we can't use commonparser.identifier to parse a type name,
    // because that one is meant to be used for identifier used by the
    // user, and excludes keywords such as "int", "string" etc.
    chset<> identchar( "a-zA-Z-_0-9" );
    RULE( type_name, lexeme_d[ alpha_p >> *identchar ] );

    constantdefinition =
        "const"
        // the type
            >> expect_type( type_name[bind( &ValueChangeParser::seentype, this, _1, _2 ) ])
            >> constdecl[bind( &ValueChangeParser::seenconstantdefinition, this )] 
            >> *(ch_p(',') >> constdecl[bind( &ValueChangeParser::seenconstantdefinition, this )] );
                                                                                   

    aliasdefinition =
         "alias"
         // the type
         >> expect_type(type_name [ bind( &ValueChangeParser::seentype, this, _1, _2 ) ])
         // next the name for the alias
         >> expect_ident( commonparser.identifier[ bind( &ValueChangeParser::storedefinitionname, this, _1, _2 ) ])
         >> expect_ais( ch_p('=') )
         // and a value to assign to it
         >> expect_init( expressionparser.parser() )[ bind( &ValueChangeParser::seenaliasdefinition, this ) ];

    variabledefinition =
         "var"
             >> expect_type( type_name[bind( &ValueChangeParser::seentype, this, _1, _2 ) ])
             >> vardecl[bind( &ValueChangeParser::seenvariabledefinition, this ) ]
             >> *(ch_p(',') >> vardecl[bind( &ValueChangeParser::seenvariabledefinition, this ) ] );
    
    variableassignment = 
        "set" >> expect_change(variablechange);

    /**
     * One of the most important parsers in the ValueChangeParser. Variable assignment
     * syntax is defined here, what can be on the left side of the = sign :
     */
    variablechange =
        ( peerparser.locator()[ bind( &ValueChangeParser::storepeername, this) ] // traverse peers
          >> propparser.locator()[ bind( &ValueChangeParser::seenproperty, this)] // traverse propertybags
          // notasserting will just 'fail' to parse.
          >> commonparser.notassertingidentifier[ bind( &ValueChangeParser::storename, this, _1, _2 ) ] // final variable after last '.'
          >> !( '[' >> expect_index( expressionparser.parser() ) >>  ch_p(']')
                [ bind( &ValueChangeParser::seenindexassignment, this) ]) // subindex final variable
          >> ch_p( '=' )
          >> expect_expr( expressionparser.parser()) )[ bind( &ValueChangeParser::seenvariableassignment, this ) ];

    paramdefinition =
        "param"
            >> expect_type( type_name[bind( &ValueChangeParser::seentype, this, _1, _2 ) ])
            >> baredecl[bind( &ValueChangeParser::seenbaredefinition, this ) ]
            >> *(ch_p(',') >> baredecl[bind( &ValueChangeParser::seenbaredefinition, this ) ] );

    baredefinition =
         type_name[ bind( &ValueChangeParser::seentype, this, _1, _2 )]
          >> baredecl[bind( &ValueChangeParser::seenbaredefinition, this )];

    baredecl =
        expect_ident( commonparser.identifier[ bind( &ValueChangeParser::storedefinitionname, this, _1, _2 )] )
            >> !( ch_p('(') >> expect_integer( expressionparser.parser()[bind( &ValueChangeParser::seensizehint, this)]) >> expect_close( ch_p(')')) ) ;

    vardecl =
        baredecl >> !( ch_p('=') >> expect_init( expressionparser.parser() ) );

    constdecl =
        baredecl >> expect_cis( ch_p('=') ) >> expect_init( expressionparser.parser() );

  }

    TaskContext* ValueChangeParser::setStack( TaskContext* tc )
    {
        //cerr << "VCP : Stack set to " << tc->getName() <<endl;
        context = tc;
        // DO NOT SET CONTEXT of PEERPARSER TO tc. 
        // Peerparser operates in main task context and not on stack.
        peerparser.setContext( tc );
        return expressionparser.setStack( tc );
    }

    TaskContext* ValueChangeParser::setContext( TaskContext* tc )
    {
        //cerr << "VCP : Context set to " << tc->getName() <<endl;
        context = tc;
        peerparser.setContext( tc );
        return expressionparser.setContext( tc );
    }

    void ValueChangeParser::seensizehint()
    {
        DataSourceBase::shared_ptr expr = expressionparser.getResult();
        expressionparser.dropResult();
        assert( expr.get() );
        //assert( !expressionparser.hasResult() );
        DataSource<int>::shared_ptr i = dynamic_cast<DataSource<int>* >( expr.get() );
        std::string typen = type->getType();
        if ( i.get() == 0 ) {
            this->cleanup();
            throw parse_exception_semantic_error
                ("Attempt to initialize "+typen+" "+valuename+" with a "+expr->getType()+", expected an integer expression." );
        }
        if ( i->get() < 0 ) {
            std::stringstream value;
            value << i->get();
            this->cleanup();
            throw parse_exception_semantic_error
                ("Attempt to initialize "+typen+" "+valuename+" with an expression leading to a negative number "+value.str()
                 +". Initialization expressions are evaluated once at parse time !" );
        }
        if ( i->get() == 0 ) {
            ORO_CoreLib::Logger::log() << Logger::Warning <<
                "Attempt to initialize "<<typen<<" "<<valuename<<" with an expression leading to zero (0)"
                                       <<". Initialization expressions are evaluated once at parse time !" << Logger::endl;
        }
        sizehint = i->get();
    }

  void ValueChangeParser::seenconstantdefinition()
  {
    DataSourceBase::shared_ptr expr = expressionparser.getResult();
    expressionparser.dropResult();
    //assert( !expressionparser.hasResult() );
    TaskAttributeBase* var;
    Logger::log() << Logger::Info << "Building "<<type->getType() <<" "<<valuename; // rest is filled in by buildConstant().
      if (sizehint == -1 )
          var = type->buildConstant(expr);
      else {
          var = type->buildConstant(expr, sizehint);
      }
      if ( var == 0 ) // bad assignment.
          {
              Logger::log() << " failed !"<<Logger::endl;
              this->cleanup();
              throw parse_exception_semantic_error
                  ("Attempt to initialize a const "+type->getType()+" with a "+expr->getType()+"." );
          }

    context->attributeRepository.setValue( valuename, var );
    definedvalues.push_back( var );
    parseddefnames.push_back( valuename );
  }

  void ValueChangeParser::storedefinitionname( iter_t begin, iter_t end )
  {
    std::string name( begin, end );
    if ( context->attributeRepository.isDefined( name ) ) {
        this->cleanup();
        throw parse_exception_semantic_error( "Identifier \"" + name +
                                              "\" is already defined." );
    }
    valuename = name;
  }

  void ValueChangeParser::seentype( iter_t begin, iter_t end )
  {
    std::string name( begin, end );
    type = typerepos->type( name );
    if ( type == 0 )
      throw parse_exception_semantic_error( "\"" + name + "\" is an unknown type..." );
  }

  void ValueChangeParser::seenaliasdefinition()
  {
    DataSourceBase::shared_ptr expr = expressionparser.getResult();
    expressionparser.dropResult();
    //assert( !expressionparser.hasResult() );
    TaskAttributeBase* alias;
    alias = type->buildAlias( expr.get() );
    if ( ! alias ) {
        this->cleanup();
        throw parse_exception_semantic_error(
        "Attempt to define an alias of type "+type->getType()+" to an expression of type "+expr->getType()+"." );
    }
    context->attributeRepository.setValue( valuename, alias );
    definedvalues.push_back( alias );
    parseddefnames.push_back( valuename );
    CommandInterface* nc(0);
    assigncommands.push_back( nc );
  }

    void ValueChangeParser::cleanup()
    {
        for_each(assigncommands.begin(), assigncommands.end(), bind(delete_ptr(), _1));
    }

  void ValueChangeParser::seenbaredefinition()
  {
    // type has been stored by calling 'seentype'
    // valuename has been stored by calling 'storename'
      TaskAttributeBase* var;
      if (sizehint == -1 )
          var = type->buildVariable();
      else {
          var = type->buildVariable(sizehint);
      }
      context->attributeRepository.setValue( valuename, var );
      definedvalues.push_back( var );
      parseddefnames.push_back( valuename );
  }

    void ValueChangeParser::seenproperty() {
        //nop, ask propparser lateron. propparser is reset by this->reset()
    }

    void ValueChangeParser::storename( iter_t begin, iter_t end ) {
        valuename = std::string( begin, end );
    }

  void ValueChangeParser::storepeername()
  {
      //cerr << "seen peer " << peerparser.peer()->getName()<<endl;
      //peername  = peerparser.peer();
      // the peerparser.object() should contain "this"
      //peerparser.reset();
      // reset the Property parser to traverse this peers bag :
      propparser.setPropertyBag( peerparser.peer()->attributeRepository.properties() ); // may be null. ok.
  }

  void ValueChangeParser::seenvariabledefinition()
  {
      // build type.
      TaskAttributeBase* var;
      if (sizehint == -1 )
          var = type->buildVariable();
      else {
          var = type->buildVariable(sizehint);
      }
      context->attributeRepository.setValue( valuename, var );
      definedvalues.push_back( var );
      parseddefnames.push_back( valuename );

      if ( expressionparser.hasResult() ) {
          DataSourceBase::shared_ptr expr = expressionparser.getResult();
          expressionparser.dropResult();
          //assert( !expressionparser.hasResult() );
          try {
              CommandInterface* ac = var->assignCommand( expr.get(), true );
              assigncommands.push_back( ac );
          }
          catch( const bad_assignment& e ) {
              this->cleanup();
              throw parse_exception_semantic_error
                  ( "Attempt to initialize a var "+var->toDataSource()->getType()+" with a "+ expr->getType() + "." );
          }
      }
  }

  void ValueChangeParser::seenvariableassignment()
  {
      TaskAttributeBase* var = 0;
      PropertyBase*     prop = 0;

      peername = peerparser.peer();
      peerparser.reset();
 
      // if bag is non-null, 'valuename' must be one of its properties :
      if ( propparser.bag() && propparser.property() ) {
          // propparser.property() is the Property<PropertyBag> of a nested bag() :
          // valuename is the element of this bag.
          // we need to use Property assignment commands instead of
          // taskattributebase assignment commands lateron.
          prop = propparser.bag()->find( valuename );
          if ( prop == 0 )
              throw parse_exception_semantic_error( "In "+context->getName()+": Property \"" + valuename + "\" not defined in nested PropertyBag '"+propparser.property()->getName()+"' of TaskContext '"+peername->getName()+"'." );

          propparser.reset();
      } else {
          // not a property case :
          var = peername->attributeRepository.getValue( valuename );
          // SIDENOTE: now, we must be sure that if this program gets copied,
          // the DS still points to the peer's attribute, and not to a new copy. TaskAttribute and Properties
          // takes care of this by definition, but the variable of a loaded StateMachine or program
          // must first get an instantiation-copy() before they become uncopyable. 
          if ( !var )
              throw parse_exception_semantic_error(  "In "+context->getName()+": Attribute \"" + valuename + "\" not defined in task '"+peername->getName()+"'." );
      }

      // collect RHS :
      DataSourceBase::shared_ptr expr = expressionparser.getResult();
      expressionparser.dropResult();
      //assert( !expressionparser.hasResult() );

      if ( index_ds && prop ) {
          throw parse_exception_semantic_error(
              "Cannot use index with Property<"+prop->getType()+"> " + valuename + " inside PropertyBag. Not Implemented. Add Propery as TaskAttribute to allow index assignment." );
      }

      if ( index_ds && var ) {
          CommandInterface* ac;
          try {
              ac = var->assignIndexCommand( index_ds.get(), expr.get() );
              assigncommands.push_back( ac );
          }
          catch( const bad_assignment& e) {
              // type-error :
              throw parse_exception_semantic_error(
                    "Impossible to assign "+valuename+"[ "+index_ds->getType()+" ] to value of type "+expr->getType()+".");
          }
          // not allowed :
          if ( !ac )
              throw parse_exception_semantic_error(
                     "Cannot use index with constant, alias or non-indexed value \"" + valuename + "\"." );
      } 
      if ( !index_ds && var) {
        try {
            CommandInterface* assigncommand = var->assignCommand( expr.get(), false );
            assigncommands.push_back(assigncommand);
            // if null, not allowed.
            if ( ! assigncommand )
                throw parse_exception_semantic_error( "Cannot set constant or alias \"" + valuename + "\" in TaskContext "+ peername->getName()+"." );
        }
        catch( const bad_assignment& e )
            {
                // type-error :
                throw parse_exception_semantic_error
                    ( "Attempt to assign variable of type "+var->toDataSource()->getType()+" with a "+ expr->getType() + "." );
            }
      }
      if ( !index_ds && prop) {
          CommandInterface* assigncommand = prop->refreshCommand( expr.get() );
          assigncommands.push_back(assigncommand);
          if ( ! assigncommand ) {
              throw parse_exception_semantic_error( "Cannot set Property<"+ prop->getType() +"> " + valuename + " to value of type "+expr->getType()+"." );
          }
      }

      // allow to restart over...
      index_ds = 0;
  }

  void ValueChangeParser::seenindexassignment()
  {
    index_ds = expressionparser.getResult();
    expressionparser.dropResult();
    assert(index_ds);
    //assert( !expressionparser.hasResult() );
  }

  void ValueChangeParser::reset()
  {
    assigncommands.clear();
    definedvalues.clear();
    parseddefnames.clear();
    valuename = "";
    type = 0;
    index_ds = 0;
    peername = 0;
    sizehint = -1;
    peerparser.reset();
    propparser.reset();
  }

  rule_t& ValueChangeParser::constantDefinitionParser()
  {
    return constantdefinition;
  }

  rule_t& ValueChangeParser::aliasDefinitionParser()
  {
    return aliasdefinition;
  }

  rule_t& ValueChangeParser::variableDefinitionParser()
  {
    return variabledefinition;
  }

  rule_t& ValueChangeParser::variableAssignmentParser()
  {
    return variableassignment;
  }

  rule_t& ValueChangeParser::variableChangeParser()
  {
    return variablechange;
  }

  rule_t& ValueChangeParser::paramDefinitionParser()
  {
    return paramdefinition;
  }

  rule_t& ValueChangeParser::bareDefinitionParser()
  {
    return baredefinition;
  }
}
